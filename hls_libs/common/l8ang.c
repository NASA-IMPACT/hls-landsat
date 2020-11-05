#include "l8ang.h"

/* Read scene-based L8 sun/view zenith/azimuth angle into a structure described in l8ang_t.
 * This is needed for the gridding of the scene-based angle into MGRS tiles.
 *
 * There are four types of angles in four input files: solar zenith, solar azimuth,
 * band 4 (red) view zenith and azimuth. USGS chooes the angles of the red band to represent
 * all spectral bands. 
 *
 * The angle is produced by USGS in COG format, and HLS converts it to ENVI binary 
 * for this code to read, for example:
 * convert 
 *   LC08_L1TP_140041_20130503_20200330_02_T1_SZA.TIF
 * to
 *   LC08_L1TP_140041_20130503_20200330_02_T1_SZA.img
 * The parameter filename of this function is the filename of SZA, from which filenames 
 * for SZA, VZA, VAA can be inferred. The ENVI header filename is inferred from the SZA
 * filename too and opened for geolocation.
 *
 * A related function is open_l8ang(l8ang_t  *l8ang, intn access_mode) which
 * reads or writes the angle in the tiled format described in l8ang.h.
 * 
 * Changes made to USGS angles:
 * 1. USGS code saves azimuth that is greater 180 as (azimuth-360). This code adds 360 to
 *    negative azimuth. 
 * 2. USGS uses 0 as the fill value, which is not a good choice. Change it to 40,000 (after
 *    scaling).
 */
int read_l8ang_inpathrow(l8ang_t  *l8ang, char *fname_sz) 
{
	char fname[500];     /* angle filename for SA,VZ,VA is derived from fname_sz */	
	
	char message[MSGLEN];
	FILE *fang;
	char *base, *cp;
	char header[500];
	char line[500];
	FILE *fhdr;
	int nrow, ncol, i;
	int16 tmp;


	/* Read the header file of the solar zneith angle image */
	strcpy(header, fname_sz);
	base = strrchr(header, '/');
	if (base == NULL)
		base = header;
	else
		base++;
	cp = strstr(base, ".img");
	strcpy(cp, ".hdr");
	if ((fhdr = fopen(header, "r")) == NULL) {
		sprintf(message, "Cannot open %s", header);
		Error(message);
		return(1);
	}
	while (fgets(line, sizeof(line), fhdr)) {
		if (strstr(line, "lines")) {
			cp = strrchr(line, '=');
			l8ang->nrow = atoi(cp+1);
		}
		if (strstr(line, "samples")) {
			cp = strrchr(line, '=');
			l8ang->ncol = atoi(cp+1);
		}
		if (strstr(line, "map info")) {
			int zone;
			/* map info = {UTM, 1.0, 1.0, 219885.000000, 4426515.000000, 30.000000, 30.000000, 18, North, WGS-84, units=Meters} */
			/* ULX and ULY for the angles created by USGS refers to the pixel UL corner,
			 * not the pixel center as in the TOA files.
			 */
			cp = strchr(line, ','); 
			cp = strchr(cp+1, ','); 
			cp = strchr(cp+1, ','); 
			/* Read after 3rd comma. Gdal-translate has adjusted the UL coordinates by half a pixel. */
		        l8ang->ulx = atof(cp+1); 
			cp = strchr(cp+1, ','); 
		        l8ang->uly = atof(cp+1); /* Read after 4th comma */
			cp = strchr(cp+1, ','); 
			cp = strchr(cp+1, ','); 
			cp = strchr(cp+1, ','); 
			zone = atoi(cp+1);	  /* Read after 7th comma */
			if (strstr(line, "North"))
				sprintf(l8ang->numhem, "%dN", zone);
			else
				sprintf(l8ang->numhem, "%dS", zone);

			break;
		}
	}
	fclose(fhdr);
	nrow = l8ang->nrow;
	ncol = l8ang->ncol;

	/* Memory for all SDS in l8ang */
	if ((l8ang->sz = (uint16*)calloc(nrow * ncol, sizeof(uint16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	if ((l8ang->sa = (uint16*)calloc(nrow * ncol, sizeof(uint16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	if ((l8ang->vz = (uint16*)calloc(nrow * ncol, sizeof(uint16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	if ((l8ang->va = (uint16*)calloc(nrow * ncol, sizeof(uint16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	
	/* Solar zenith.  */
	strcpy(fname, fname_sz);
	if ((fang = fopen(fname, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname);
		Error(message);
		return(1);
	}
	/* It is correct to read int16 zenith as uint16 */
	if (fread(l8ang->sz, sizeof(uint16), nrow * ncol, fang) != nrow * ncol) {
		sprintf(message, "Input file is too short: %s", fname);
		Error(message);
		return(1);
	}
	if (fseek(fang, 1, SEEK_CUR) != 0) {
		sprintf(message, "Input file is too long: %s", fname);
		Error(message);
		return(1);
	}
	fclose(fang);

	/* Solar azimuth */
	strcpy(fname, fname_sz);
	base = strrchr(fname, '/');
	if (base == NULL)
		base = fname;
	else
		base++;
	cp = strstr(base, "_SZA.img");
	if (cp == NULL) {
		fprintf(stderr, "Filename does not end with _SZA.img: %s\n", fname_sz);
		return(1);
	}
	strcpy(cp, "_SAA.img");

	if ((fang = fopen(fname, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname);
		Error(message);
		return(1);
	}
	for (i = 0; i < nrow * ncol; i++) {
		if (fread(&tmp, sizeof(int16), 1, fang) != 1) {
			sprintf(message, "Input file is too short: %s", fname);
			Error(message);
			return(1);
		}
		if (tmp < 0)
			l8ang->sa[i] = tmp + 36000;
		else
			l8ang->sa[i] = tmp;
			
	}
	if (fseek(fang, 1, SEEK_CUR) != 0) {
		sprintf(message, "Input file is too long: %s", fname);
		Error(message);
		return(1);
	}
	fclose(fang);

	/* View zenith */
	strcpy(cp, "_VZA.img");
	if ((fang = fopen(fname, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname);
		Error(message);
		return(1);
	}
	/* It is correct to read int16 zenith as uint16 */
	if (fread(l8ang->vz, sizeof(int16), nrow * ncol, fang) != nrow * ncol) {
		sprintf(message, "Input file is too short: %s", fname);
		Error(message);
		return(1);
	}
	if (fseek(fang, 1, SEEK_CUR) != 0) {
		sprintf(message, "Input file is too long: %s", fname);
		Error(message);
		return(1);
	}
	fclose(fang);

	/* View azimuth */
	strcpy(cp, "_VAA.img");
	if ((fang = fopen(fname, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname);
		Error(message);
		return(1);
	}
	for (i = 0; i < nrow * ncol; i++) {
		if (fread(&tmp, sizeof(int16), 1, fang) != 1) {
			sprintf(message, "Input file is too short: %s", fname);
			Error(message);
			return(1);
		}
		if (tmp < 0)
			l8ang->va[i] = tmp + 36000;
		else
			l8ang->va[i] = tmp;
			
	}
	if (fseek(fang, 1, SEEK_CUR) != 0) {
		sprintf(message, "Input file is too long: %s", fname);
		Error(message);
		return(1);
	}
	fclose(fang);

	/* Oct 23, 2020. USGS uses 0 as fill value; change the fill value to ANGFILL (40000) */
	for (i = 0; i < nrow * ncol; i++) {
		/* When all four angles for a pixel are 0, the pixel is outside the swath. Azimuth
		 * can be 0 even within the swath. */
		if ( l8ang->sz[i] == USGS_ANGFILL && 
		     l8ang->sa[i] == USGS_ANGFILL && 
		     l8ang->vz[i] == USGS_ANGFILL && 
		     l8ang->va[i] == USGS_ANGFILL) {
			l8ang->sz[i] = ANGFILL;
			l8ang->sa[i] = ANGFILL;
			l8ang->vz[i] = ANGFILL;
			l8ang->va[i] = ANGFILL;
		}
	}

	return(0);
}

/* Open L8 angle HDF for read/write/create.  For tiled angle.  */
int open_l8ang(l8ang_t  *l8ang, intn access_mode) 
{
	char sds_name[500];     
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index;
	char attr_name[200];
	int32 dimsizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];
	int32 count;

	char message[MSGLEN];
	l8ang->access_mode = access_mode;

	l8ang->sz = NULL;
	l8ang->sa = NULL;
	l8ang->vz = NULL;
	l8ang->va = NULL;

	/* For DFACC_READ or DFACC_WRITE, find the image dimension from band 1.
	 * For DFACC_CREATE, image dimension is directly given. 
	 */
	if (access_mode == DFACC_READ || access_mode == DFACC_WRITE) {
		if ((l8ang->sd_id = SDstart(l8ang->fname, access_mode)) == FAIL) {
			sprintf(message, "Cannot open %s", l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* Solar zenith */
		strcpy(sds_name, L8_SZ);
		if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}
		l8ang->sds_id_sz = SDselect(l8ang->sd_id, sds_index);

		if (SDgetinfo(l8ang->sds_id_sz, sds_name, &rank, dimsizes, &data_type, &nattr) == FAIL) {
			Error("Error in SDgetinfo");
			return(ERR_READ);
		}
		l8ang->nrow = dimsizes[0];
		l8ang->ncol = dimsizes[1];
		start[0] = 0; edge[0] = l8ang->nrow;
		start[1] = 0; edge[1] = l8ang->ncol;

		if ((l8ang->sz = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}

		if (SDreaddata(l8ang->sds_id_sz, start, NULL, edge, l8ang->sz) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* Solar azimuth */
		strcpy(sds_name, L8_SA);
		if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}
		l8ang->sds_id_sa = SDselect(l8ang->sd_id, sds_index);

		if ((l8ang->sa = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}

		if (SDreaddata(l8ang->sds_id_sa, start, NULL, edge, l8ang->sa) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* view zenith */
		/* All bands have the same view zenith and azimuth angle */
		strcpy(sds_name, L8_VZ);
		if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}
		l8ang->sds_id_vz = SDselect(l8ang->sd_id, sds_index);

		if ((l8ang->vz = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}

		if (SDreaddata(l8ang->sds_id_vz, start, NULL, edge, l8ang->vz) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* view azimuth */
		strcpy(sds_name, L8_VA);
		if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}
		l8ang->sds_id_va = SDselect(l8ang->sd_id, sds_index);

		if ((l8ang->va = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}

		if (SDreaddata(l8ang->sds_id_va, start, NULL, edge, l8ang->va) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* sceneID */
		if ((attr_index = SDfindattr(l8ang->sd_id, L1TSCENEID)) != FAIL) {
			if (SDattrinfo(l8ang->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
				Error("Error in SDattrinfo");
				return(-1);
			}
			if (SDreadattr(l8ang->sd_id, attr_index, l8ang->l1tsceneid) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", L1TSCENEID, l8ang->fname);
				Error(message);
			}
			l8ang->l1tsceneid[count] = '\0'; 	/* Be sure to make it a string */
		}
		else 
			l8ang->l1tsceneid[0] = '\0'; 	/* Empty string*/

		/* Read the ENVI header file, which is created because certain attributes are
		 * not set in some files (e.g. atmospheric correction output).
		 */
		char header[500];
		sprintf(header, "%s.hdr", l8ang->fname);
		read_envi_utm_header(header, l8ang->numhem, &l8ang->ulx, &l8ang->uly);
	}
	else if (access_mode == DFACC_CREATE) {
		char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
		int32 comp_type;   /*Compression flag*/
		comp_info c_info;  /*Compression structure*/
		comp_type = COMP_CODE_DEFLATE;
		c_info.deflate.level = 2;     /*Level 9 would be too slow */
		rank = 2;
		long k, npix;

		dimsizes[0] = l8ang->nrow;
		dimsizes[1] = l8ang->ncol;
		if ((l8ang->sd_id = SDstart(l8ang->fname, access_mode)) == FAIL) {
			sprintf(message, "Cannot create %s", l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		npix = l8ang->nrow * l8ang->ncol;

		/* Solar zenith */
		strcpy(sds_name, L8_SZ);
		if ((l8ang->sds_id_sz = SDcreate(l8ang->sd_id, sds_name, DFNT_UINT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_sz, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_sz, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_sz, comp_type, &c_info);	
		if ((l8ang->sz = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		for (k = 0; k < npix; k++) 
			l8ang->sz[k] = ANGFILL;
			
		/* Solar azimuth */
		strcpy(sds_name, L8_SA);
		if ((l8ang->sds_id_sa = SDcreate(l8ang->sd_id, sds_name, DFNT_UINT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_sa, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_sa, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_sa, comp_type, &c_info);	
		if ((l8ang->sa = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		for (k = 0; k < npix; k++) 
			l8ang->sa[k] = ANGFILL;

		/* View zenith and azimuth*/
		strcpy(sds_name, L8_VZ);
		if ((l8ang->sds_id_vz = SDcreate(l8ang->sd_id, sds_name, DFNT_UINT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_vz, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_vz, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_vz, comp_type, &c_info);	
		/* SDS attribute not complete. OK */

		/* memory */
		if ((l8ang->vz = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		for (k = 0; k < npix; k++) 
			l8ang->vz[k] = ANGFILL;	// Finally changed to 40,000


		strcpy(sds_name, L8_VA);
		if ((l8ang->sds_id_va = SDcreate(l8ang->sd_id, sds_name, DFNT_UINT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_va, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_va, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_va, comp_type, &c_info);	
		/* SDS attribute not complete. OK */

		/* memory */
		if ((l8ang->va = (uint16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(uint16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		for (k = 0; k < npix; k++) 
			l8ang->va[k] = ANGFILL;


		l8ang->l1tsceneid[0] = '\0'; 	/* Empty string*/
	}

	return 0;
} 


/* Add L1T sceneID as an attribute. A sceneID can be added for the first time for an HDF;
 * a second scene can fall on a S2 tile too.
 *
 * The actual write takes place in the close_l8ang function. Good or bad design? July 21, 2016.
 */
int l8ang_add_l1tsceneid(l8ang_t *l8ang, char *l1tsceneid)
{
	int len;
	
	len = strlen(l8ang->l1tsceneid);
	if (len == 0)
		strcpy(l8ang->l1tsceneid, l1tsceneid);	 /* Not check the out of bound error yet */
	else { 
		l8ang->l1tsceneid[len] = ';';  /* separated by ';' */
		strcpy(&(l8ang->l1tsceneid[len+1]), l1tsceneid);	 /* Not check the out of bound error yet */
	}

	return 0;
}

/* close tile-based angle file*/
int close_l8ang(l8ang_t *l8ang)
{
	int ib;
	char message[MSGLEN];
		
	if ((l8ang->access_mode == DFACC_CREATE || l8ang->access_mode == DFACC_WRITE) && l8ang->sd_id != FAIL) {
		if (! l8ang->tile_has_data) {
			SDend(l8ang->sd_id);
			l8ang->sd_id = FAIL;
			fprintf(stderr, "Delete empty tile: %s\n", l8ang->fname);
			remove(l8ang->fname);
		}
		else {
			int32 start[2], edge[2];
			start[0] = 0; edge[0] = l8ang->nrow;
			start[1] = 0; edge[1] = l8ang->ncol;

			/* Solar zenith and azimuth */
			if (SDwritedata(l8ang->sds_id_sz, start, NULL, edge, l8ang->sz) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(l8ang->sds_id_sz);

			if (SDwritedata(l8ang->sds_id_sa, start, NULL, edge, l8ang->sa) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(l8ang->sds_id_sa);

			/* View zenith and azimuth */
			if (SDwritedata(l8ang->sds_id_vz, start, NULL, edge, l8ang->vz) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(l8ang->sds_id_vz);

			if (SDwritedata(l8ang->sds_id_va, start, NULL, edge, l8ang->va) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(l8ang->sds_id_va);
	
			/* L1T scene ID */
			/* When string l8ang->l1tsceneid is not set,  the output file is silently corrupted.  @2:22am, Apr 16, 2016. */
			if (strlen(l8ang->l1tsceneid) > 0) {
				if (SDsetattr(l8ang->sd_id, L1TSCENEID, DFNT_CHAR8, strlen(l8ang->l1tsceneid), (VOIDP)l8ang->l1tsceneid) == FAIL) {
					Error("Error in SDsetattr");
					return(0);
				}
			}
	
			SDend(l8ang->sd_id);
			l8ang->sd_id = FAIL;
	
			/* Add an ENVI header */
			char fname_hdr[500];
			sprintf(fname_hdr, "%s.hdr", l8ang->fname);
			add_envi_utm_header(l8ang->numhem, 
						l8ang->ulx, 
						l8ang->uly, 
						l8ang->nrow, 
						l8ang->ncol, 
						HLS_PIXSZ,
						fname_hdr);
		}
	}

	/* free up memory */
	if (l8ang->sz != NULL) {
		free(l8ang->sz);
		l8ang->sz = NULL;
	}
	if (l8ang->sa != NULL) {
		free(l8ang->sa);
		l8ang->sa = NULL;
	}
	if (l8ang->vz != NULL) {
		free(l8ang->vz);
		l8ang->vz = NULL;
	}

	if (l8ang->va != NULL) {
		free(l8ang->va);
		l8ang->va = NULL;
	}
	return 0;
}
