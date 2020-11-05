#include "l8ang.h"

/* Modification on Sep 7, 2017, Nov 20, 2017 */

/* Read scene-based L8 angle, into a structure described in l8ang_t.
 * This is needed for the tiling of solar/view angle.
 *
 * This function is explicitly designed to read scene-based angle which can 
 * be in two different formats due to the co-existence of pre-collection and 
 * collection data.  The input type is determined from the input filename. 
 *
 * The input file can be a single angle hdf for the pre-collection data, or the 
 * band 1 solar angle file in plain binary format for the collection data, from
 * which view angle filenames for other bands can be derived.  For the collection
 * data, a UGGS tool computes solar angle for all spectral bands, but they are 
 * identifical. Each USGS plain binary file contains azimuth band and zenith band.
 * Both formats have an ENVI header file which provides map info.
 *
 * After reading, the l8ang instance will contain solar zenith and azimuth, which
 * is same for all bands, and band-specific view zenith and azimuth SDS for all bands
 * except pan and cirrus, as prescribed in l8ang.h.  Since the pre-collection 
 * angle input only has three SDS: solar zenith, band-independent view zenith, and 
 * relative azimuth, the output solar zenith is the same as the input, solar azimuth 
 * is in fact the input relative azimuth, the view zenith for all bands is a replication
 * of input band-independent view zenith, and view azimuth is set to 0 for all bands 
 * as required by the relative azimuth input.
 *
 * A related function open_l8ang(l8ang_t  *l8ang, intn access_mode) 
 * reads or writes the angle in the "standard" format described in l8ang.h.
 * 
 * Sep 7, 2017
 * USGS code saves azimuth that is greater 180 as (azimuth-360). And it can be inferred that
 * Martin's matlab code uses the same convention and saves relative azimuth as solar minus view.
 */
int read_l8ang_inpathrow(l8ang_t  *l8ang, char *fname_ang) 
{
	char header[500];
	FILE *fhdr;
	char line[500];
	char collection; /* Whether the input is collection based */
	char *cp;
	int ib;
	int32 start[2], edge[2];
	int nrow, ncol;
	char message[MSGLEN];

	/* From the basename of the input file, determine input type */
	cp = strrchr(fname_ang, '/');
	if (cp == NULL)
		cp = fname_ang;
	if (strstr(cp, "_L1") && strstr(cp, ".img"))
		collection = 1;
	else if (strstr(cp, "_geometry.hdf"))
		collection = 0;
	else {
		sprintf(message, "Cannot determine input file type: %s", fname_ang);
		Error(message);
		return(1);
	}

	/* Read header for any type of input */
	/* Note: There is a function in util.c that reads ENVI header, but it does not
	 * read the dimension of the image, since Sentinel-2 image dimensions vary
	 * with bands. So rewrite. Not a good design.
 	 */ 
	sprintf(header, "%s.hdr", fname_ang);
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
		        l8ang->ulx = atof(cp+1); /* Read after 3rd comma */
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
	ncol = l8ang->ncol;   /* To type less later on*/
	
	/* Memory for all SDS in l8ang */
	if ((l8ang->sz = (int16*)calloc(nrow * ncol, sizeof(int16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	if ((l8ang->sa = (int16*)calloc(nrow * ncol, sizeof(int16))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	/* No pan, no cirrus */
	for (ib = 0; ib < L8NRB-1; ib++) {
		if ((l8ang->vz[ib] = (int16*)calloc(nrow * ncol, sizeof(int16))) == NULL) {
			Error("Cannot allocate memory");
			return(1);
		}
		if ((l8ang->va[ib] = (int16*)calloc(nrow * ncol, sizeof(int16))) == NULL) {
			Error("Cannot allocate memory");
			return(1);
		}
	}
	
	/* Read geometry */
	if (collection) {
		/********************  Collection based *******************/
		/*
		LC08_L1TP_019034_20150117_20170302_01_T1_solar_B01.img
		LC08_L1TP_019034_20150117_20170302_01_T1_solar_B01.img.hdr
		LC08_L1TP_019034_20150117_20170302_01_T1_sensor_B01.img
		LC08_L1TP_019034_20150117_20170302_01_T1_sensor_B01.img.hdr
		*/
		char fname_solar_b1[500];
		char fname[500]; 	/* A specific angle file name */
		FILE *fang;

		strcpy(fname_solar_b1, fname_ang); /* It is b1 solar */

		/* Solar azimuth and zenith. */
		/* Read from b1 solar, but should be the same for all bands.  */
		/* Note: Azimuth before zenith in each file; a bit odd */
		strcpy(fname, fname_solar_b1);
		if ((fang = fopen(fname, "r")) == NULL) {
			sprintf(message, "Cannot open %s", fname);
			Error(message);
			return(1);
		}
		if (fread(l8ang->sa, sizeof(int16), nrow * ncol, fang) != nrow * ncol) {
			sprintf(message, "Input file size wrong: %s", fname);
			Error(message);
			return(1);
		}
		if (fread(l8ang->sz, sizeof(int16), nrow * ncol, fang) != nrow * ncol) {
			sprintf(message, "Input file size wrong: %s", fname);
			Error(message);
			return(1);
		}

		/* View azimuth and zenith for each band */
		/* No pan, no cirrus */
		for (ib = 0; ib < L8NRB-1; ib++) {
			strcpy(fname, fname_solar_b1);
			cp = strrchr(fname, '/');	
			if (cp == NULL)
				cp = fname;
			cp = strstr(cp, "_solar_B01");
			sprintf(cp, "_sensor_B0%d.img", ib+1);

			if ((fang = fopen(fname, "r")) == NULL) {
				sprintf(message, "Cannot open %s", fname);
				Error(message);
				return(1);
			}
			if (fread(l8ang->va[ib], sizeof(int16), nrow * ncol, fang) != nrow * ncol) {
				sprintf(message, "Input file size wrong: %s", fname);
				Error(message);
				return(1);
			}
			if (fread(l8ang->vz[ib], sizeof(int16), nrow * ncol, fang) != nrow * ncol) {
				sprintf(message, "Input file size wrong: %s", fname);
				Error(message);
				return(1);
			}
			fclose(fang);
		}
	}
	else {	
		/********************  pre-ollection based *******************/
		/* HDF, input geometry of all bands have been assumed identical; replicate view geometry 
		 * for each band, but also consider that only relative azimuth is available in input*/
		int sd_id, sds_id[3]; 
		char sds_name[500]; 
		int sds_index;
		int isds;
		int16 *ang[3];

		start[0] = 0; edge[0] = nrow;
		start[1] = 0; edge[1] = ncol;

		if ((sd_id = SDstart(fname_ang, DFACC_READ)) == FAIL) {
			sprintf(message, "Cannot open %s", fname_ang);
			Error(message);
			return(ERR_READ);
		}
		for (isds = 0; isds < 3; isds++) {
			strcpy(sds_name, L8_ANG_SDS_NAME_PC[isds]);
			if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, fname_ang);
				Error(message);
				return(ERR_READ);
			}
			sds_id[isds] = SDselect(sd_id, sds_index);

			if ((ang[isds] = (int16*)calloc(nrow * ncol, sizeof(int16))) == NULL) {
				Error("Cannot allocate memory");
				return(1);
			}

			if (SDreaddata(sds_id[isds], start, NULL, edge, ang[isds]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, fname_ang);
				Error(message);
				return(ERR_READ);
			}
		}

		/* Solar zenith */
		memcpy(l8ang->sz, ang[0], nrow * ncol *2);
		/* Solar azimuth set to relative azimuth */ 
		memcpy(l8ang->sa, ang[2], nrow * ncol *2); 

		/* View zenith is replicated for each band, view zenith to 0 */
		for (ib = 0; ib < L8NRB-1; ib++) {
			memcpy(l8ang->vz[ib], ang[1], nrow * ncol *2); 
			memset(l8ang->va[ib], '\0',   nrow * ncol *2);  
		}

		free(ang[0]);
		free(ang[1]);
		free(ang[2]);
	}

	return(0);
}

/* Open L8 angle HDF for read/write/create. The input contain per-band view angles
 * and common solar angles. Deals with tiled angle.
 */
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

	int ib;
	char message[MSGLEN];
	l8ang->access_mode = access_mode;

	l8ang->sz = NULL;
	l8ang->sa = NULL;
	for (ib = 0; ib < L8NRB-1; ib++) {
		l8ang->vz[ib] = NULL;
		l8ang->va[ib] = NULL;
	}

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

		if ((l8ang->sz = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
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

		if ((l8ang->sa = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}

		if (SDreaddata(l8ang->sds_id_sa, start, NULL, edge, l8ang->sa) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
			Error(message);
			return(ERR_READ);
		}

		/* view zenith */
		/* No pan, no cirrus */
		for (ib = 0; ib < L8NRB-1; ib++) {
			strcpy(sds_name, L8_VZ[ib]);
			if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
				Error(message);
				return(ERR_READ);
			}
			l8ang->sds_id_vz[ib] = SDselect(l8ang->sd_id, sds_index);

			if ((l8ang->vz[ib] = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
				fprintf(stderr, "Cannot allocate memory\n");
				exit(1);
			}

			if (SDreaddata(l8ang->sds_id_vz[ib], start, NULL, edge, l8ang->vz[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
				Error(message);
				return(ERR_READ);
			}
		}

		/* view azimuth */
		/* No pan, no cirrus */
		for (ib = 0; ib < L8NRB-1; ib++) {
			strcpy(sds_name, L8_VA[ib]);
			if ((sds_index = SDnametoindex(l8ang->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, l8ang->fname);
				Error(message);
				return(ERR_READ);
			}
			l8ang->sds_id_va[ib] = SDselect(l8ang->sd_id, sds_index);

			if ((l8ang->va[ib] = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
				fprintf(stderr, "Cannot allocate memory\n");
				exit(1);
			}

			if (SDreaddata(l8ang->sds_id_va[ib], start, NULL, edge, l8ang->va[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, l8ang->fname);
				Error(message);
				return(ERR_READ);
			}
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
		if ((l8ang->sds_id_sz = SDcreate(l8ang->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_sz, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_sz, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_sz, comp_type, &c_info);	
		if ((l8ang->sz = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		/* Apr 5, 2017: Forgot to initialize */
		for (k = 0; k < npix; k++) 
			l8ang->sz[k] = LANDSAT_ANGFILL;
			
		/* Solar azimuth */
		strcpy(sds_name, L8_SA);
		if ((l8ang->sds_id_sa = SDcreate(l8ang->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) { 
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}    
		PutSDSDimInfo(l8ang->sds_id_sa, dimnames[0], 0);
		PutSDSDimInfo(l8ang->sds_id_sa, dimnames[1], 1);
		SDsetcompress(l8ang->sds_id_sa, comp_type, &c_info);	
		if ((l8ang->sa = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
			fprintf(stderr, "Cannot allocate memory\n");
			exit(1);
		}
		for (k = 0; k < npix; k++) 
			l8ang->sa[k] = LANDSAT_ANGFILL;

		/* View zenith and azimuth*/
		/* No pan, no cirrus */
		for (ib = 0; ib < L8NRB-1; ib++) {
			strcpy(sds_name, L8_VZ[ib]);
			if ((l8ang->sds_id_vz[ib] = SDcreate(l8ang->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) { 
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(l8ang->sds_id_vz[ib], dimnames[0], 0);
			PutSDSDimInfo(l8ang->sds_id_vz[ib], dimnames[1], 1);
			SDsetcompress(l8ang->sds_id_vz[ib], comp_type, &c_info);	
			/* SDS attribute not complete. OK */

			/* memory */
			if ((l8ang->vz[ib] = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
				fprintf(stderr, "Cannot allocate memory\n");
				exit(1);
			}

			/* Apr 5, 2017: Forgot to initialize */
			for (k = 0; k < npix; k++) 
				l8ang->vz[ib][k] = LANDSAT_ANGFILL;


			strcpy(sds_name, L8_VA[ib]);
			if ((l8ang->sds_id_va[ib] = SDcreate(l8ang->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) { 
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(l8ang->sds_id_va[ib], dimnames[0], 0);
			PutSDSDimInfo(l8ang->sds_id_va[ib], dimnames[1], 1);
			SDsetcompress(l8ang->sds_id_va[ib], comp_type, &c_info);	
			/* SDS attribute not complete. OK */

			/* memory */
			if ((l8ang->va[ib] = (int16*)calloc(l8ang->nrow * l8ang->ncol, sizeof(int16))) == NULL) {
				fprintf(stderr, "Cannot allocate memory\n");
				exit(1);
			}

			/* Apr 5, 2017: Forgot to initialize */
			for (k = 0; k < npix; k++) 
				l8ang->va[ib][k] = LANDSAT_ANGFILL;
		}


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
			for (ib = 0; ib < L8NRB-1; ib++) {
				if (SDwritedata(l8ang->sds_id_vz[ib], start, NULL, edge, l8ang->vz[ib]) == FAIL) {
					Error("Error in SDwritedata");
					return(ERR_CREATE);
				}
				SDendaccess(l8ang->sds_id_vz[ib]);

				if (SDwritedata(l8ang->sds_id_va[ib], start, NULL, edge, l8ang->va[ib]) == FAIL) {
					Error("Error in SDwritedata");
					return(ERR_CREATE);
				}
				SDendaccess(l8ang->sds_id_va[ib]);
			}
	
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
	for (ib = 0; ib < L8NRB-1; ib++) {
		if (l8ang->vz[ib] != NULL) {
			free(l8ang->vz[ib]);
			l8ang->vz[ib] = NULL;
		}

		if (l8ang->va[ib] != NULL) {
			free(l8ang->va[ib]);
			l8ang->va[ib] = NULL;
		}
	}
	return 0;
}
