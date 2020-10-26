#include <stdio.h>
#include <string.h>

#include "lsatmeta.h"
#include "util.h"

/* Return the address of the first character in the attribute value, and
 * the number of characters in the attribute value. If the attribute
 * value is quoted, remove the quotes.
 * Jan 22, 2017: So easy to make mistakes with pointers. The original
 * function declaration was following, no way of passing the *cb back.
 * void mtlval(char *line, char *cb, int *nc)
 */
char * mtlval(char *line, int *nc)
{
	char *cb;
	int i = 0;

	while (line[i] != '=')
		i++;
	i++; /* skip '=' */	
	while (line[i] == ' ' || line[i] == '\t' || line[i] == '"')
		i++;
	cb = line+i;
	
	*nc = 0;
	while (line[i] != '"' && line[i] != '\n') { 
		(*nc)++;
		i++;
	}
	return (cb);
}

int read_mtl(lsatmeta_t *lsatmeta, char *mtl)
{
	FILE *fmtl;
	char line[500];
	char *cb;	/* The position of the first character in the attribute value */
	int nc;		/* The number of characters in the attribute value */	

	char acqdate[100], centime[100];	/* From MTL. To be combined into sensing-time */
	char proj[20], datum[20];		/* To be combined */
	int utmzone;
	char ssm_model_found = 0;
	char ssm_position_found = 0;
	char productid_found = 0; 	/* product ID not available in pre-collection */

	char message[MSGLEN];

	if ((fmtl = fopen(mtl, "r")) == NULL) {
		sprintf(message, "Cannot open %s", mtl);	
		Error(message);
		return(-1);
	} 
	/******** First, read from MTL *****/
	/* Comment on Jan 18, 2017:  attribute names from the MTL are used directly in reading;
	 * the defined attribute names by "#define" are used for output as they can be slightly
	 * different from those used in MTL.
	 */

	
	// Added on Oct 22, 2020. Initialize all the char array so that if the metadata
	// names changes, we will see rather than crush the output HDF.
	char * filler = "UNKNOWN";
	strcpy(lsatmeta->sceneid, filler);
	strcpy(lsatmeta->productid, filler);
	strcpy(lsatmeta->l1proctime, filler);
	strcpy(lsatmeta->software, filler);		
	strcpy(lsatmeta->datatype, filler);		
	strcpy(lsatmeta->sensor, filler);		
	strcpy(acqdate, filler);
	strcpy(lsatmeta->sensing_time, filler);
	strcpy(lsatmeta->ssm_model, filler); 
	strcpy(lsatmeta->ssm_position, filler);
	strcpy(proj, filler);
	strcpy(datum, filler);
	strcpy(lsatmeta->cs_name, filler);

	while (fgets(line, sizeof(line), fmtl)) {
		if (strstr(line, "LANDSAT_SCENE_ID")) {
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->sceneid, cb, nc);
			lsatmeta->sceneid[nc] = '\0';
		}
		if (strstr(line, "LANDSAT_PRODUCT_ID")) {
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->productid, cb, nc);
			lsatmeta->productid[nc] = '\0';
		}
		// else if (strstr(line, "FILE_DATE")) { 	/* This was for C1.  Oct 22, 2020 */
		else if (strstr(line, "DATE_PRODUCT_GENERATED")) { 	
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->l1proctime, cb, nc);
			lsatmeta->l1proctime[nc] = '\0';
		}
		else if (strstr(line, "PROCESSING_SOFTWARE_VERSION")) {
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->software, cb, nc);
			lsatmeta->software[nc] = '\0';	
		}
		//else if (strstr(line, "DATA_TYPE")) { 	/* This was for C1 */
		else if (strstr(line, "PROCESSING_LEVEL")) { 
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->datatype, cb, nc);
			lsatmeta->datatype[nc] = '\0';
		}
		if (strstr(line, "SENSOR_ID")) { 
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->sensor, cb, nc);
			lsatmeta->sensor[nc] = '\0';
		}
		else if (strstr(line, "DATE_ACQUIRED")) {
			cb = mtlval(line, &nc);
			strncpy(acqdate, cb, nc);
			acqdate[nc] = '\0';
		}
		else if (strstr(line, "SCENE_CENTER_TIME")) {
			cb = mtlval(line, &nc);
			strncpy(centime, cb, nc);
			centime[nc] = '\0';
			sprintf(lsatmeta->sensing_time, "%sT%s", acqdate, centime);	/* Combine data and time */
		}
		else if (strstr(line, "CORNER_UL_PROJECTION_X_PRODUCT")) {
			cb = mtlval(line, &nc);
			lsatmeta->ululx = atof(cb) - 15;  /* adjust by half pixel */
		}
		else if (strstr(line, "CORNER_UL_PROJECTION_Y_PRODUCT")) {
			cb = mtlval(line, &nc);
			lsatmeta->ululy = atof(cb) + 15;  /* adjust by half pixel */
		}
		else if (strstr(line, "REFLECTIVE_LINES")) {
			cb = mtlval(line, &nc);
			lsatmeta->nr = atoi(cb);
		}
		else if (strstr(line, "REFLECTIVE_SAMPLES")) {
			cb = mtlval(line, &nc);
			lsatmeta->nc = atoi(cb);
		}
		else if (strstr(line, "SUN_AZIMUTH")) {
			cb = mtlval(line, &nc);
			lsatmeta->msa[0] = atof(cb);
		}
		else if (strstr(line, "SUN_ELEVATION")) {
			cb = mtlval(line, &nc);
			lsatmeta->msz[0] = 90 - atof(cb);
		}
		else if (strstr(line, "TIRS_SSM_MODEL")) {
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->ssm_model, cb, nc);
			lsatmeta->ssm_model[nc] = '\0'; 
		}
		else if (strstr(line, "TIRS_SSM_POSITION_STATUS")) {
			cb = mtlval(line, &nc);
			strncpy(lsatmeta->ssm_position, cb, nc);
			lsatmeta->ssm_position[nc] = '\0'; 
		}
		else if (strstr(line, "MAP_PROJECTION")) {
			cb = mtlval(line, &nc);
			strncpy(proj, cb, nc);
			proj[nc] = '\0';
		}
		else if (strstr(line, "DATUM")) {
			cb = mtlval(line, &nc);
			strncpy(datum, cb, nc);
			datum[nc] = '\0';
		}
		else if (strstr(line, "UTM_ZONE")) {
			cb = mtlval(line, &nc);
			utmzone = atoi(cb);
			sprintf(lsatmeta->cs_name, "%s, %s, UTM ZONE %d", proj, datum, utmzone);
		}
		else if (strstr(line, "GRID_CELL_SIZE_REFLECTIVE")) {
			cb = mtlval(line, &nc);
			lsatmeta->spatial_resolution = atof(cb);

			/* Have read all items; stop now*/
			// Oct 22, 2020: Do not break; the order of metadata elements may change.
			// break;
		}
	}



	return(0);
}

/*
*/

int write_input_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	/* Be careful:
	 * 
	 * Write an unterminated character array can ruin the whole file. Make sure every 
	 * character attribute contains a valid character string.
	 */
	int ret;
	
	/* L_SENSOR */
	ret = SDsetattr(lsat->sd_id, L_SENSOR, DFNT_CHAR8, strlen(lsatmeta->sensor), (VOIDP)lsatmeta->sensor);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_SCENEID */
	ret = SDsetattr(lsat->sd_id, L_SCENEID, DFNT_CHAR8, strlen(lsatmeta->sceneid), (VOIDP)lsatmeta->sceneid);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_PRODUCTID */
	ret = SDsetattr(lsat->sd_id, L_PRODUCTID, DFNT_CHAR8, strlen(lsatmeta->productid), (VOIDP)lsatmeta->productid);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_DATA_TYPE */
	ret = SDsetattr(lsat->sd_id, L_DATA_TYPE, DFNT_CHAR8, strlen(lsatmeta->datatype), (VOIDP)lsatmeta->datatype);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_SENSING_TIME */
	ret = SDsetattr(lsat->sd_id, L_SENSING_TIME, DFNT_CHAR8, strlen(lsatmeta->sensing_time), (VOIDP)lsatmeta->sensing_time);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/*  L_L1PROCTIME */
	ret = SDsetattr(lsat->sd_id, L_L1PROCTIME, DFNT_CHAR8, strlen(lsatmeta->l1proctime), (VOIDP)lsatmeta->l1proctime);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_USGS_SOFTWARE */
	ret = SDsetattr(lsat->sd_id, L_USGS_SOFTWARE, DFNT_CHAR8, strlen(lsatmeta->software), (VOIDP)lsatmeta->software);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_HORIZONTAL_CS_NAME */
	ret = SDsetattr(lsat->sd_id, L_HORIZONTAL_CS_NAME, DFNT_CHAR8, strlen(lsatmeta->cs_name), (VOIDP)lsatmeta->cs_name);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_NROWS */
	ret = SDsetattr(lsat->sd_id, L_NROWS, DFNT_INT32, 1, (VOIDP)&lsatmeta->nr);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_NCOLS */
	ret = SDsetattr(lsat->sd_id, L_NCOLS, DFNT_INT32, 1, (VOIDP)&lsatmeta->nc);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_ULX */
	ret = SDsetattr(lsat->sd_id, L_ULX, DFNT_FLOAT64, 1, (VOIDP)&lsatmeta->ululx);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_ULY */
	ret = SDsetattr(lsat->sd_id, L_ULY, DFNT_FLOAT64, 1, (VOIDP)&lsatmeta->ululy);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_SPATIAL_RESOLUTION */
	ret = SDsetattr(lsat->sd_id, L_SPATIAL_RESOLUTION, DFNT_FLOAT64, 1, (VOIDP)&lsatmeta->spatial_resolution);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_MSZ */
	ret = SDsetattr(lsat->sd_id, L_MSZ, DFNT_FLOAT64, lsatmeta->nscene, (VOIDP)lsatmeta->msz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_MSA */
	ret = SDsetattr(lsat->sd_id, L_MSA, DFNT_FLOAT64, lsatmeta->nscene, (VOIDP)lsatmeta->msa);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_TIRS_SSM_MODEL */
	ret = SDsetattr(lsat->sd_id, L_TIRS_SSM_MODEL,  DFNT_CHAR8, strlen(lsatmeta->ssm_model), (VOIDP)lsatmeta->ssm_model);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_TIRS_SSM_POSITION_STATUS */
	ret = SDsetattr(lsat->sd_id, L_TIRS_SSM_POSITION_STATUS,  DFNT_CHAR8, strlen(lsatmeta->ssm_position), (VOIDP)lsatmeta->ssm_position);
	if (ret != 0) {
		Error("Error in SDsetattr");
		return(-1);
	}

	/* L_ACCODE */
	if (strlen(lsatmeta->accode) > 0) {
		ret = SDsetattr(lsat->sd_id, L_ACCODE,  DFNT_CHAR8, strlen(lsatmeta->accode), (VOIDP)lsatmeta->accode);
		if (ret != 0) {
			Error("Error in SDsetattr");
			return(-1);
		}
	}

	return(0);
}

/* Metadata entirely from a scene (not a tiled Landsat image). This is for the atmospheric correction output */
int set_input_metadata(lsat_t *lsat, char *mtl)
{
	int ret;

	lsatmeta_t lsatmeta;

	
	ret = read_mtl(&lsatmeta, mtl);
	if (ret != 0) {
		Error("Error in read_mtl");
		exit(1);
	}
	
	/* Nov 21, 2017. ACCODE attribute is set elsewhere. Set a flag (.i.e., null string) 
	 * for write_input_metadata to ignore it  */
	lsatmeta.accode[0] = '\0';
	lsatmeta.nscene = 1; /* Since this is for AC output, only one scene (not for a tile where they there may be two of the same day */
	ret = write_input_metadata(lsat, &lsatmeta);
	if (ret != 0) {
		Error("write_input_metadata");
		return(-1);
	}


	/* Oct 21, 2016:
	 * For the atmospheric correction output, ULX, ULY, zonehem are not available (we no longer rely on
 	 * an ENVI header for this information, although an ENVI header will be created for each step after
	 * atmospheric correction.
	 *
   	 * ululx and ululy  means adjustment to the UL corner of the UL pixel, although 
   	 * ENVI and USGS use pixel center for reference, we use UL corner.
	 */
	char *cpos,  zonehem[10];
	int zone;
	lsat->ulx = lsatmeta.ululx;
	lsat->uly = lsatmeta.ululy; 
	cpos = strrchr(lsatmeta.cs_name, ' ');
	zone = atoi(cpos+1);
	sprintf(lsat->zonehem, "%d%s", zone, lsat->uly > 0 ? "N" : "S");

	return(0);
}

/* Get the HDF metadata that are originally from MTL.
 * Addition on Nov 21, 2017: L_ACCODE for atmospheric correction code.
 */
int get_input_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	int32 nattr, attr_index;
	int count;
	char attr_name[200];
	int32 data_type;
	char message[MSGLEN];

	/* SENSOR */
	strcpy(attr_name, L_SENSOR);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->sensor) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->sensor[count] = '\0';

	/* L_SCENEID */
	strcpy(attr_name, L_SCENEID);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->sceneid) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->sceneid[count] = '\0';

	/* L_PRODUCTID */
	strcpy(attr_name, L_PRODUCTID);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->productid) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->productid[count] = '\0';

	/* L_DATA_TYPE */
	strcpy(attr_name, L_DATA_TYPE);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->datatype) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->datatype[count] = '\0';

	/* SENSING_TIME*/
	strcpy(attr_name, L_SENSING_TIME);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->sensing_time) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->sensing_time[count] = '\0';


	/* L1PROCTIME */
	strcpy(attr_name, L_L1PROCTIME);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->l1proctime) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->l1proctime[count] = '\0';

	/* SOFTWARE */		
	strcpy(attr_name, L_USGS_SOFTWARE);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->software) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->software[count] = '\0';

	/* HORIZONTAL_CS_NAME*/
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
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->cs_name) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->cs_name[count] = '\0';


	/* NROWS */   
	strcpy(attr_name, L_NROWS);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, &lsatmeta->nr) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* NCOLS*/   
	strcpy(attr_name, L_NCOLS);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, &lsatmeta->nc) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* SPATIAL_RESOLUTION*/   
	strcpy(attr_name, L_SPATIAL_RESOLUTION);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, &lsatmeta->spatial_resolution) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* ULX */
	strcpy(attr_name, L_ULX);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, &lsatmeta->ululx) == FAIL) {
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
	if (SDreadattr(lsat->sd_id, attr_index, &lsatmeta->ululy) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}


	/* MSZ*/   
	strcpy(attr_name, L_MSZ);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->msz) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* MSA*/   
	strcpy(attr_name, L_MSA);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->msa) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* TIRS_SSM_MODEL */
	strcpy(attr_name, L_TIRS_SSM_MODEL);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->ssm_model) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->ssm_model[count] = '\0';

	/* TIRS_SSM_POSITION_STATUS_STATUS  */
	strcpy(attr_name, L_TIRS_SSM_POSITION_STATUS );
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->ssm_position) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->ssm_position[count] = '\0';


	/* L_ACCODE */
	strcpy(attr_name, L_ACCODE);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->accode) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->accode[count] = '\0';

	return(0);
}

/* Copy some of the input scene  metadata during the tiling of a landsat scene. 
   But use the tile's ULX, ULY and dimension info  
*/
int copy_input_metadata_to_tile(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	int ret;
	lsatmeta->ululx = lsat->ulx;
	lsatmeta->ululy = lsat->uly;
	lsatmeta->nr = lsat->nrow;
	lsatmeta->nc = lsat->ncol;
	
	lsatmeta->nscene = 1;
	// fprintf(stderr, "debug: accode = %s, len = %d\n", lsatmeta->accode, (int)(strlen(lsatmeta->accode)));
	ret = write_input_metadata(lsat, lsatmeta);
	if (ret != 0) {
		Error("write_input_metadata");
		return(-1);
	}

	return(0);
}

/* When two scenes of the same day fall on the same tile (i.e., same path, adjacent row, same day),
 * update the input metadate by concatenating. 
 */
int update_input_metadata_for_tile(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	int ret;

	lsatmeta_t newmeta;
	ret = get_input_metadata(lsat, &newmeta); 
	if (ret != 0) {
		Error("get_input_metadata");
		return(-1);
	}
	
	char tmpstr[500];

	/* sensor */
	sprintf(tmpstr, "%s; %s", newmeta.sensor, lsatmeta->sensor);
	strcpy(newmeta.sensor, tmpstr);

	/* SCENEID */
	sprintf(tmpstr, "%s; %s", newmeta.sceneid, lsatmeta->sceneid);
	strcpy(newmeta.sceneid, tmpstr);

	/* PRODUCTID */
	sprintf(tmpstr, "%s; %s", newmeta.productid, lsatmeta->productid);
	strcpy(newmeta.productid, tmpstr);

	/* data type */
	sprintf(tmpstr, "%s; %s", newmeta.datatype, lsatmeta->datatype);
	strcpy(newmeta.datatype, tmpstr);

	/* Sensing time */
	sprintf(tmpstr, "%s; %s", newmeta.sensing_time, lsatmeta->sensing_time);
	strcpy(newmeta.sensing_time, tmpstr);

	/* l1proctime */
	sprintf(tmpstr, "%s; %s", newmeta.l1proctime, lsatmeta->l1proctime);
	strcpy(newmeta.l1proctime, tmpstr);

	/* projection */
	sprintf(tmpstr, "%s; %s", newmeta.cs_name, lsatmeta->cs_name);
	strcpy(newmeta.cs_name, tmpstr);

	/* MSZ and MSZ */
	newmeta.msz[1] = lsatmeta->msz[0];
	newmeta.msa[1] = lsatmeta->msa[0];

	/* thermal model */
	sprintf(tmpstr, "%s; %s", newmeta.ssm_model, lsatmeta->ssm_model);
	strcpy(newmeta.ssm_model, tmpstr);

	/* thermal position*/
	sprintf(tmpstr, "%s; %s", newmeta.ssm_position, lsatmeta->ssm_position);
	strcpy(newmeta.ssm_position, tmpstr);

	/* ACCODE */
	sprintf(tmpstr, "%s; %s", newmeta.accode, lsatmeta->accode);
	strcpy(newmeta.accode, tmpstr);
	
	newmeta.nscene = 2;
	ret = write_input_metadata(lsat, &newmeta);
	if (ret != 0) {
		Error("write_input_metadata");
		return(-1);
	}

	return(0);
}

/* Never used??? Dec 26, 2016*/
int write_arop_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	/* Be careful:
	 * 
	 * Write an unterminated character array can ruin the whole file. Make sure every 
	 * character attribute contains a valid character string.
	 */

	SDsetattr(lsat->sd_id, L_AROP_BASE_SENTINEL2, DFNT_CHAR8, strlen(lsatmeta->s2basename), (VOIDP)lsatmeta->s2basename);
	SDsetattr(lsat->sd_id, L_AROP_NCP, DFNT_UINT32, lsatmeta->nscene, (VOIDP)lsatmeta->ncp);
	SDsetattr(lsat->sd_id, L_AROP_RMSE, DFNT_FLOAT64, lsatmeta->nscene, (VOIDP)lsatmeta->rmse);
	SDsetattr(lsat->sd_id, L_AROP_XSHIFT, DFNT_FLOAT64, lsatmeta->nscene, (VOIDP)lsatmeta->addtox);
	SDsetattr(lsat->sd_id, L_AROP_YSHIFT, DFNT_FLOAT64, lsatmeta->nscene, (VOIDP)lsatmeta->addtoy);
	
	return(0);
}


/* There is only one scene on the tile so far */
int set_arop_metadata(lsat_t *lsat,  
			char *s2basename, 
			int ncp, 
			double rmse, 
			double shiftx, 
			double shifty) 
{

	lsatmeta_t lsatmeta;

	lsatmeta.nscene = 1;

	strcpy(lsatmeta.s2basename, s2basename);
	lsatmeta.ncp[0] = ncp;
	lsatmeta.rmse[0] = rmse;
	lsatmeta.addtox[0] = shiftx;
	lsatmeta.addtoy[0] = shifty;

	write_arop_metadata(lsat, &lsatmeta);
	
	return(0);
}


/* Read the AROP metadata */
int get_arop_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta)
{
	int32 nattr, attr_index;
	int count;
	char attr_name[200];
	int32 data_type;
	char message[MSGLEN];

	/* L_AROP_BASE_SENTINEL2 */
	strcpy(attr_name, L_AROP_BASE_SENTINEL2);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDattrinfo(lsat->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
		Error("Error in SDattrinfo");
		return(-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->s2basename) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}
	lsatmeta->s2basename[count] = '\0';

	/* L_AROP_NCP*/   
	strcpy(attr_name, L_AROP_NCP);
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->ncp) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* L_AROP_RMSE */
	strcpy(attr_name, L_AROP_RMSE); 
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->rmse) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* L_AROP_XSHIFT */
	strcpy(attr_name, L_AROP_XSHIFT); 
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->addtox) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}

	/* L_AROP_YSHIFT */
	strcpy(attr_name, L_AROP_YSHIFT); 
	if ((attr_index = SDfindattr(lsat->sd_id, attr_name)) == FAIL) {
		sprintf(message, "Attribute \"%s\" not found in %s", attr_name, lsat->fname);
		Error(message);
		return (-1);
	}
	if (SDreadattr(lsat->sd_id, attr_index, lsatmeta->addtoy) == FAIL) {
		sprintf(message, "Error read attribute \"%s\" in %s", attr_name, lsat->fname);
		Error(message);
		return(-1);
	}


	return(0);

}

/* Update AROP metadata when the second scene comes in */
int update_arop_metadata(lsat_t *lsat, 
			char *s2basename, 
			int ncp, 
			double rmse, 
			double shiftx, 
			double shifty) 
{
	char tmpstr[500];

	lsatmeta_t newmeta;
	get_arop_metadata(lsat, &newmeta);

	/* S2 basename */
	/* Apr 12, 2017: reference image is the same for a tile; no need to update */
	//sprintf(tmpstr, "%s; %s", newmeta.s2basename, s2basename);
	//strcpy(newmeta.s2basename, tmpstr);

	/* Other numerical items */
	newmeta.nscene = 2;
	newmeta.ncp[1] = ncp;
	newmeta.rmse[1] = rmse;
	newmeta.addtox[1] = shiftx;
	newmeta.addtoy[1] = shifty;

	write_arop_metadata(lsat, &newmeta);
	
	return(0);
}


/* The NBAR metadata */
int write_nbar_solarzenith(lsat_t *lsat, double nbarsz)
{
	int ret; 
	ret = SDsetattr(lsat->sd_id, L_NBARSZ, DFNT_FLOAT64, 1, (VOIDP)&nbarsz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	return(0);
}

int write_mean_angle(lsat_t *lsat, double msz, double msa, double mvz, double mva)
/* Mar 31, 2020 */
{
	int ret; 
	/* L_MSZ */
	ret = SDsetattr(lsat->sd_id, L_MSZ, DFNT_FLOAT64, 1, (VOIDP)&msz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	/* L_MSA */
	ret = SDsetattr(lsat->sd_id, L_MSA, DFNT_FLOAT64, 1, (VOIDP)&msa);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	/* L_MVZ */
	ret = SDsetattr(lsat->sd_id, L_MVZ, DFNT_FLOAT64, 1, (VOIDP)&mvz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}
	/* L_MVA */
	ret = SDsetattr(lsat->sd_id, L_MVA, DFNT_FLOAT64, 1, (VOIDP)&mva);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	return(0);
}
