#include "lsat.h"

/* open L8 sr for read, create, or write (update) */
int open_lsat(lsat_t *lsat, intn access_mode)
{
	char sds_name[500];     
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index;
	int32 dimsizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];
	char attr_name[200];
	int32 count;

	int irow, icol;
	int ib;
	char message[MSGLEN];

	lsat->access_mode = access_mode;

	for (ib = 0; ib < L8NRB; ib++) 
		lsat->ref[ib] = NULL;
	for (ib = 0; ib < 2; ib++) 
		lsat->thm[ib] = NULL;
	lsat->accloud = NULL;
	lsat->acmask = NULL;
	lsat->fmask = NULL;
	lsat->brdfflag = NULL;

	int sngi; 		/* SDS name-group index.  An awkward term. */
	/* The input is either atmospheric correction output, which uses its own 
	 * set of SDS names, or any subsequent processing output, which uses 
	 * a more "standard" set of SDS names. These two groups have index
	 * 0 and 1 respectively in the first dimension of L8_REF_SDS_NAME[][].
	 * 						  L8_THM_SDS_NAME[][]
	 * 
 	 * Band 2 is used to detect which group is used, since in the AC output
         * band 2 is named "band02-blue" while in subsequent problem is simpley "B02".
	 */

	/* Allocate memory to hold the SDS for any access mode.
	 * For DFACC_READ or DFACC_WRITE, find the image dimension, while
	 * for DFACC_CREATE, image dimension is directly given. 
	 */
	if (lsat->access_mode == DFACC_READ || lsat->access_mode == DFACC_WRITE) {
		if ((sd_id = SDstart(lsat->fname, lsat->access_mode)) == FAIL) {
			sprintf(message, "Cannot open %s", lsat->fname);
			Error(message);
			return(ERR_READ);
		}

		/* The input is either atmospheric correction output, which uses its own 
		 * set of SDS names (sngi == 0), or any subsequent processing output, which uses 
		 * a more "standard" set of SDS names (sngi == 1). Detect which group is used.
		 */
		sngi = 0; 	/* Assuming output is from AC for the moment*/
		strcpy(sds_name, L8_REF_SDS_NAME[0][1]);	/* "band02-blue" */
		if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
			strcpy(sds_name, L8_REF_SDS_NAME[1][1]);	/* "B02" */
			if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
				sprintf(message, "Neither %s nor %s are found in in %s", 
						  L8_REF_SDS_NAME[0][0], L8_REF_SDS_NAME[1][0], lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			sngi = 1; 	/* output from processes subsequent to AC */
		}
		sds_id = SDselect(sd_id, sds_index);
		if (SDgetinfo(sds_id, sds_name, &rank, dimsizes, &data_type, &nattr) == FAIL) {
			Error("Error in SDgetinfo");
			return(ERR_READ);
		} 
		SDendaccess(sds_id);
		SDend(sd_id);
		lsat->nrow = dimsizes[0];
		lsat->ncol = dimsizes[1];
	}

	for (ib = 0; ib < L8NRB; ib++) {
		if ((lsat->ref[ib] = (int16*)calloc(lsat->nrow * lsat->ncol, sizeof(int16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
	}
	for (ib = 0; ib < L8NTB; ib++) {
		if ((lsat->thm[ib] = (int16*)calloc(lsat->nrow * lsat->ncol, sizeof(int16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
	}

	/* When LaSRC CLOUD is available, ACmask and Fmask haven't been created yet*/
	if (strcmp(lsat->ac_cloud_available, AC_CLOUD_AVAILABLE) == 0) {
		if ((lsat->accloud = (uint8*)calloc(lsat->nrow * lsat->ncol, sizeof(uint8))) == NULL) {
			Error("Cannot allocate memory\n");
			exit(1);
		}
	}
	else {
		/* ACmask */
		if ((lsat->acmask = (uint8*)calloc(lsat->nrow * lsat->ncol, sizeof(uint8))) == NULL) {
			Error("Cannot allocate memory\n");
			exit(1);
		}

		/* Fmask */
		if ((lsat->fmask = (uint8*)calloc(lsat->nrow * lsat->ncol, sizeof(uint8))) == NULL) {
			Error("Cannot allocate memory\n");
			exit(1);
		}
	}

	/* If it is NBAR.  Jun 14: should be set as an hdf attribute */
	if (strcmp(lsat->is_nbar, IS_NBAR) == 0) {
		if ((lsat->brdfflag = (uint8*)calloc(lsat->nrow * lsat->ncol, sizeof(uint8))) == NULL) {
			Error("Cannot allocate memory");
			return(1);
		}
	}
		
	/********** Read or Write*/
	if (access_mode == DFACC_READ || access_mode == DFACC_WRITE) {
		start[0] = 0; edge[0] = lsat->nrow;
		start[1] = 0; edge[1] = lsat->ncol;

		if ((lsat->sd_id = SDstart(lsat->fname, access_mode)) == FAIL) {
			sprintf(message, "Cannot open %s", lsat->fname);
			Error(message);
			return(ERR_READ);
		}

		/* Reflectance bands  */
		for (ib = 0; ib < L8NRB; ib++) {
			strcpy(sds_name, L8_REF_SDS_NAME[sngi][ib]);
			if ((sds_index = SDnametoindex(lsat->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			lsat->sds_id_ref[ib] = SDselect(lsat->sd_id, sds_index);

			if (SDreaddata(lsat->sds_id_ref[ib], start, NULL, edge, lsat->ref[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
		}

		/* Thermal bands */
		for (ib = 0; ib < 2; ib++) {
			strcpy(sds_name, L8_THM_SDS_NAME[sngi][ib]);
			if ((sds_index = SDnametoindex(lsat->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			lsat->sds_id_thm[ib] = SDselect(lsat->sd_id, sds_index);
			if (SDreaddata(lsat->sds_id_thm[ib], start, NULL, edge, lsat->thm[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
		}

		if (strcmp(lsat->ac_cloud_available, AC_CLOUD_AVAILABLE) == 0) {
			/* This is output directly from LaSRC*/
			strcpy(sds_name, L8_CLOUD_SDS_NAME);
			if ((sds_index = SDnametoindex(lsat->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			lsat->sds_id_accloud = SDselect(lsat->sd_id, sds_index);
			if (SDreaddata(lsat->sds_id_accloud, start, NULL, edge, lsat->accloud) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			SDendaccess(lsat->sds_id_accloud);
		}
		else {
			/* ACmask*/
			strcpy(sds_name, ACMASK_NAME);
			if ((sds_index = SDnametoindex(lsat->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			lsat->sds_id_acmask = SDselect(lsat->sd_id, sds_index);
			if (SDreaddata(lsat->sds_id_acmask, start, NULL, edge, lsat->acmask) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}

			/* Fmask */
			strcpy(sds_name, FMASK_NAME);
			if ((sds_index = SDnametoindex(lsat->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
			lsat->sds_id_fmask = SDselect(lsat->sd_id, sds_index);
			if (SDreaddata(lsat->sds_id_fmask, start, NULL, edge, lsat->fmask) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, lsat->fname);
				Error(message);
				return(ERR_READ);
			}
		}

		/* No longer rely on the ENVI header for map projection information.
		 * Read from the HDF attributes if they are available.
		 */
		if (strcmp(lsat->ulxynotset, ULXYNOTSET) == 0)
			/* Not available immediately from LaSRC.  do nothing. Set in lsatmeta.c  */ 
			; 
		else {
			char cs_name[100], *cpos;
			int zone;
			char hemi; 	/* 'N' or  'S' */
			char s2tileid[10];

			/* ULX */
			strcpy(attr_name, L_ULX);
			if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
				sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
				Error(message);
				return (-1);
			}
			if (SDreadattr(lsat->sd_id, attr_index, &lsat->ulx) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
				Error(message);
				return(-1);
			}
		
			/* ULY */
			strcpy(attr_name, L_ULY);
			if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
				sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
				Error(message);
				return (-1);
			}
			if (SDreadattr(lsat->sd_id, attr_index, &lsat->uly) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
				Error(message);
				return(-1);
			}
			/* Adjust to GCTP convention for southern hemisphere if necessary (a few lines below. Oct 3, 2018. */
		
		
			/* Set UTM zone and UTM band, for both the Landsat scene case and the Sentile-2 tile case */
			
			/* Example:  "UTM, WGS84, UTM ZONE 29" */
			/* First test whether the data is tiled in the Sentinel-2 system */
			if (strcmp(lsat->inpathrow, INWRSPATHROW) == 0) {
				/* Example metadata from hdfncdump:
				 * :HORIZONTAL_CS_NAME = "UTM, WGS84, UTM ZONE 54"
				 */
				strcpy(attr_name, L_HORIZONTAL_CS_NAME);
				if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
					sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
					Error(message);
					return (-1);
				}
				if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
					Error("Error in SDattrinfo");
					return(-1);
				}
				if (SDreadattr(lsat->sd_id, attr_index, cs_name) == FAIL) {
					sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
					Error(message);
					return(-1);
				}
				cs_name[count] = '\0';
				cpos = strrchr(cs_name, ' ');
				zone = atoi(cpos+1);
				sprintf(lsat->zonehem, "%d%s", zone, lsat->uly > 0 ? "N" : "S");
			}
			else {
				/* Should be in S2 tile */
				strcpy(attr_name, L_S2TILEID);
				if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
					sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
					Error(message);
					return (-1);
				}
				if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
					Error("Error in SDattrinfo");
					return(-1);
				}
				if (SDreadattr(lsat->sd_id, attr_index, s2tileid) == FAIL) {
					sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
					Error(message);
					return(-1);
				}
				s2tileid[count] = '\0';

				zone = atoi(s2tileid);
				if (s2tileid[2] >= 'N')
					hemi = 'N';
				else 
					hemi = 'S';
				sprintf(lsat->zonehem, "%d%c", zone, hemi);

				/* Adjust to GCTP convention for southern hemisphere if necessary. Oct 3, 2018. */
				/* This was added as a remedy to update HDF-EOS map projection information. Only needed for
				 * a few days.  In the future the adjustment will be adjusted earlier in the tiling process */
				if (strstr(lsat->zonehem, "S") && lsat->uly > 0) {
					lsat->uly -= pow(10,7);
		
					if (access_mode == DFACC_WRITE) {
        					SDsetattr(lsat->sd_id, L_ULX, DFNT_FLOAT64, 1, (VOIDP)&lsat->ulx);
        					SDsetattr(lsat->sd_id, L_ULY, DFNT_FLOAT64, 1, (VOIDP)&lsat->uly);
        				}
				}
			}
		}
	}
	else if (access_mode == DFACC_CREATE) {
		char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
		int32 comp_type;   /*Compression flag*/
		comp_info c_info;  /*Compression structure*/
		comp_type = COMP_CODE_DEFLATE;
		c_info.deflate.level = 2;     /*Level 9 would be too slow */
		rank = 2;
		dimsizes[0] = lsat->nrow;
		dimsizes[1] = lsat->ncol;

		sngi = 1;	/* Use the new set of SDS names */
		if ((lsat->sd_id = SDstart(lsat->fname, access_mode)) == FAIL) {
			sprintf(message, "Cannot create %s", lsat->fname);
			Error(message);
			return(ERR_CREATE);
		}

		/*** Reflectance bands  ***/
		for (ib = 0; ib < L8NRB; ib++) {
			/* Always use the second group of SDS names in creating new files for reflectance bands */
			strcpy(sds_name, L8_REF_SDS_NAME[sngi][ib]);

			if ((lsat->sds_id_ref[ib] = SDcreate(lsat->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    

			/* Jan 18, 2017: a function related to HDFEOS handles these too? */
			PutSDSDimInfo(lsat->sds_id_ref[ib], dimnames[0], 0);
			PutSDSDimInfo(lsat->sds_id_ref[ib], dimnames[1], 1);
			SDsetcompress(lsat->sds_id_ref[ib], comp_type, &c_info);	
			SDsetattr(lsat->sds_id_ref[ib], "long_name",  
					DFNT_CHAR8, strlen(L8_REF_SDS_LONG_NAME[ib]), (VOIDP)L8_REF_SDS_LONG_NAME[ib]);
			SDsetattr(lsat->sds_id_ref[ib], "_FillValue", DFNT_CHAR8, strlen(L8_ref_fillval), (VOIDP)L8_ref_fillval);
			SDsetattr(lsat->sds_id_ref[ib], "scale_factor", DFNT_CHAR8, 
							strlen(L8_ref_scale_factor), (VOIDP)L8_ref_scale_factor);
			SDsetattr(lsat->sds_id_ref[ib], "add_offset", DFNT_CHAR8, 
							strlen(L8_ref_add_offset), (VOIDP)L8_ref_add_offset);

			for (irow = 0; irow < lsat->nrow; irow++) {
				for (icol = 0; icol < lsat->ncol; icol++) 
					lsat->ref[ib][irow * lsat->ncol + icol] = HLS_REFL_FILLVAL;
		
			}
		}

		/*** Thermal bands ***/
		for (ib = 0; ib < L8NTB; ib++) {
			strcpy(sds_name, L8_THM_SDS_NAME[sngi][ib]);
			if ((lsat->sds_id_thm[ib] = SDcreate(lsat->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(lsat->sds_id_thm[ib], dimnames[0], 0);
			PutSDSDimInfo(lsat->sds_id_thm[ib], dimnames[1], 1);
			SDsetcompress(lsat->sds_id_thm[ib], comp_type, &c_info);	
			SDsetattr(lsat->sds_id_thm[ib], "long_name",  
					DFNT_CHAR8, strlen(L8_THM_SDS_LONG_NAME[ib]), (VOIDP)L8_THM_SDS_LONG_NAME[ib]);
			SDsetattr(lsat->sds_id_thm[ib], "_FillValue", DFNT_CHAR8, strlen(L8_thm_fillval), (VOIDP)L8_thm_fillval);
			SDsetattr(lsat->sds_id_thm[ib], "scale_factor", DFNT_CHAR8, 
							strlen(L8_thm_scale_factor), (VOIDP)L8_thm_scale_factor);
			SDsetattr(lsat->sds_id_thm[ib], "add_offset", DFNT_CHAR8, 
							strlen(L8_thm_add_offset), (VOIDP)L8_thm_add_offset);
			SDsetattr(lsat->sds_id_thm[ib], "unit", DFNT_CHAR8, strlen(L8_thm_unit), (VOIDP)L8_thm_unit);

			for (irow = 0; irow < lsat->nrow; irow++) {
				for (icol = 0; icol < lsat->ncol; icol++) 
					lsat->thm[ib][irow * lsat->ncol + icol] = HLS_THM_FILLVAL;
			}
		}


		/*** ACmask ***/
		strcpy(sds_name, ACMASK_NAME);
		if ((lsat->sds_id_acmask = SDcreate(lsat->sd_id, sds_name, DFNT_UINT8, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(lsat->sds_id_acmask, dimnames[0], 0);
		PutSDSDimInfo(lsat->sds_id_acmask, dimnames[1], 1);
		SDsetcompress(lsat->sds_id_acmask, comp_type, &c_info);	
		
		char attr[3000];
		/* Note: For better view, the blanks within the string is blank space characters, not tab */
		sprintf(attr, 	"Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
				"7-6    aerosol:\n"
				"       00 - climatology\n"
				"       01 - low\n"
				"       10 - average\n"
				"       11 - high\n"
				"5      water\n"
				"4      snow/ice\n"
				"3      cloud shadow\n"
				"2      adjacent to cloud\n"
				"1      cloud\n"
				"0      cirrus\n");
		SDsetattr(lsat->sds_id_acmask, "ACmask bit description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);
		for (irow = 0; irow < lsat->nrow; irow++) {
			for (icol = 0; icol < lsat->ncol; icol++) 
				lsat->acmask[irow * lsat->ncol + icol] = HLS_MASK_FILLVAL;
		}

		/*** Fmask ***/
		strcpy(sds_name, FMASK_NAME);
		if ((lsat->sds_id_fmask = SDcreate(lsat->sd_id, sds_name, DFNT_UINT8, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(lsat->sds_id_fmask, dimnames[0], 0);
		PutSDSDimInfo(lsat->sds_id_fmask, dimnames[1], 1);
		SDsetcompress(lsat->sds_id_fmask, comp_type, &c_info);	

		/* Note: For better view, the blanks within the string is blank space characters, not tab */
		sprintf(attr, 	"Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
				"7-6    unused\n"
				"5      water\n"
				"4      snow/ice\n"
				"3      cloud shadow\n"
				"1      cloud\n"
				"0      cirrus\n");
		SDsetattr(lsat->sds_id_fmask, "Fmask bit description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);
		for (irow = 0; irow < lsat->nrow; irow++) {
			for (icol = 0; icol < lsat->ncol; icol++) 
				lsat->fmask[irow * lsat->ncol + icol] = HLS_MASK_FILLVAL;
		}

		/* For NBAR only. Apr 22, 2016: No longer used? */
		if (strcmp(lsat->is_nbar, IS_NBAR) == 0) {
			strcpy(sds_name, BRDFFLAG_NAME);
			if ((lsat->sds_id_brdfflag = SDcreate(lsat->sd_id, sds_name, DFNT_UINT8, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}
			PutSDSDimInfo(lsat->sds_id_brdfflag, dimnames[0], 0);
			PutSDSDimInfo(lsat->sds_id_brdfflag, dimnames[1], 1);
			SDsetcompress(lsat->sds_id_brdfflag, comp_type, &c_info);	
			/* All bits are used. Can't set fill value.  Feb 22, 2016 */
			SDsetattr(lsat->sds_id_brdfflag, "_FillValue", DFNT_UINT8, 1, (VOIDP)&brdfflag_fillval);
	
			char attr[3000];
			/* Note: For better view, the blanks within the string is blank space characters, not tab */
			sprintf(attr, 	"Bits are listed from the MSB (bit 7) to the LSB (bit 0).\n"
					"1 means that band is BRDF-adjusted, 0 means not. \n"
					"Bit    Band \n"
					"7      Not used\n"
					"6      Band 07,\n"
					"5      Band 06,\n"
					"4      Band 05,\n"
					"3      Band 04,\n"
					"2      Band 03,\n"
					"1      Band 02,\n"
					"0      Band 01.\n");

			SDsetattr(lsat->sds_id_brdfflag, "brdfflag description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);
		}
	}
	else {
		sprintf(message, "access_mode not considered: %d", access_mode);
		Error(message);
		return(1);
	}

	return 0;
} 

/* Initialize the output file with input file. */
//void dup_input(lsat_t *in, lsat_t *out)
//{
//	int ib;
//	int k;
//
//	for (ib = 0; ib < L8NRB; ib++) {
//		for (k = 0; k < in->nrow * in->ncol; k++)
//			out->ref[ib][k] = in->ref[ib][k];
//	}
//	for (ib = 0; ib < 2; ib++) {
//		for (k = 0; k < in->nrow * in->ncol; k++)
//			out->thm[ib][k] = in->thm[ib][k];
//	}
//	if (strcmp(lsat->ac_cloud_available, AC_CLOUD_AVAILABLE) == 0 )
//		;
//	else {
//		/* Needed for map projection */
//		strcpy(out->zonehem, in->zonehem);
//		out->ulx = in->ulx;
//		out->uly = in->uly;
//		
//		for (k = 0; k < in->nrow * in->ncol; k++) {
//			out->acmask[k] = in->acmask[k];
//			out->fmask[k] = in->fmask[k];
//		}
//	}
//}

/* Spaital and cloud coverage in an Landsat image (scene or tile), based on Fmask  */
void lsat_setcoverage(lsat_t *lsat)
{
	int irow, icol;
	int npix, ncloud, k; 

	/* Coverage is based on broad NIR band. For S10 output, cloud cover SDS was
	 * available at 10m. So chose a 10m NIR*/
	
	npix = 0;
	ncloud = 0;
	for (irow = 0; irow < lsat->nrow; irow++) {		/* 0, 1, 2 for 10m, 20m, 60m respectively */
		for (icol = 0; icol < lsat->ncol; icol++) {
			k = irow*lsat->ncol+icol;
			if (lsat->ref[4][k] != HLS_REFL_FILLVAL) {	/* 4 is for NIR */
				npix++;
				/* cirrus, cloud, and cloud shadow */
				if ((lsat->fmask[k] & 1) == 1 || 
				    ((lsat->fmask[k] >> 1) & 1) == 1 || 
				    ((lsat->fmask[k] >> 3) & 1) == 1)
					ncloud++;
			}
		}
	}

	lsat->spcover = (int) (npix * 100.0 / (lsat->nrow * lsat->ncol) + 0.5);
	lsat->clcover = (int) (ncloud * 100.0 / npix + 0.5);

	SDsetattr(lsat->sd_id, SPCOVER, DFNT_INT16, 1, (VOIDP)&(lsat->spcover));
	SDsetattr(lsat->sd_id, CLCOVER, DFNT_INT16, 1, (VOIDP)&(lsat->clcover));
}

/* close */
int close_lsat(lsat_t *lsat)
{
	int ib;
	char message[MSGLEN];

	if (lsat->access_mode == DFACC_READ && lsat->sd_id != FAIL) {
		SDend(lsat->sd_id);
		lsat->sd_id = FAIL;
	}
	else if ((lsat->access_mode == DFACC_CREATE || lsat->access_mode == DFACC_WRITE) && lsat->sd_id != FAIL) {
		if (! lsat->tile_has_data) {
			SDend(lsat->sd_id);
			lsat->sd_id = FAIL;
			fprintf(stderr, "Delete empty tile: %s\n", lsat->fname);
			remove(lsat->fname);
		}
		else {
			int32 start[2], edge[2];
			start[0] = 0; edge[0] = lsat->nrow;
			start[1] = 0; edge[1] = lsat->ncol;

			int ret;
	
			/* Reflectance */
			for (ib = 0; ib < L8NRB; ib++) {
				if (SDwritedata(lsat->sds_id_ref[ib], start, NULL, edge, lsat->ref[ib]) == FAIL) {
					Error("Error in SDwritedata");
					return(ERR_CREATE);
				}
				SDendaccess(lsat->sds_id_ref[ib]);
			}
	
			/* Thermal */
			for (ib = 0; ib < 2; ib++) {
				if (SDwritedata(lsat->sds_id_thm[ib], start, NULL, edge, lsat->thm[ib]) == FAIL) {
					Error("Error in SDwritedata");
					return(ERR_CREATE);
				}
				SDendaccess(lsat->sds_id_thm[ib]);
			}
	
			/* ACmask */
			if (SDwritedata(lsat->sds_id_acmask, start, NULL, edge, lsat->acmask) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(lsat->sds_id_acmask);
	
			/* Fmask */
			if (SDwritedata(lsat->sds_id_fmask, start, NULL, edge, lsat->fmask) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(lsat->sds_id_fmask);
	
			/* For BRDF-adjusted reflectance */
			if (strcmp(lsat->is_nbar, IS_NBAR) == 0) {
				/* BRDF flag */
				if (SDwritedata(lsat->sds_id_brdfflag, start, NULL, edge, lsat->brdfflag) == FAIL) {
					Error("Error in SDwritedata");
					return(ERR_CREATE);
				}
				SDendaccess(lsat->sds_id_brdfflag);
			}
	
	
			/* Add an ENVI header */
			/* May 2, 2017: The header is needed not just for ENVI to show the geolocation, but
			 * also for regridding the scene-based data into an S2 tile. The map projection info
			 * is indeed part of the hdf attributes, but the ENVI header is more convenient for use
			 * for AROP since the reference image is also in ENVI format with a header.
			 */ 
			char fname_hdr[500];
			sprintf(fname_hdr, "%s.hdr", lsat->fname);
			add_envi_utm_header(lsat->zonehem, 
						lsat->ulx, 
						lsat->uly, 
						lsat->nrow, 
						lsat->ncol, 
						HLS_PIXSZ,
						fname_hdr);
	
	
	
			/* Close the file at the very end, rather than immediately following an SDS.
			 * Today I added an SDS but tried to write to it after the file was closed. 
			 */
			ret = SDend(lsat->sd_id);
			lsat->sd_id = FAIL;
			if (ret != 0) {
				sprintf(message, "Error in closing file (internal error?): %s", lsat->fname);
				return(-1);
			}
		}
	}


	/* free up memory */
	for (ib = 0; ib < L8NRB; ib++) {
		if (lsat->ref[ib] != NULL) {
			free(lsat->ref[ib]);
			lsat->ref[ib] = NULL;
		}
	}
	for (ib = 0; ib < 2; ib++) {
		if (lsat->thm[ib] != NULL) {
			free(lsat->thm[ib]);
			lsat->thm[ib] = NULL;
		}
	}
	if (lsat->accloud != NULL) {
		free(lsat->accloud);
		lsat->accloud = NULL;
	}
	if (lsat->acmask != NULL) {
		free(lsat->acmask);
		lsat->acmask = NULL;
	}
	if (lsat->fmask != NULL) {
		free(lsat->fmask);
		lsat->fmask = NULL;
	}

	return 0;
}
