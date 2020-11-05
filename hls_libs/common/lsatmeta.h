/* The Landsat metadata items.
 */
#ifndef LSAT_META_H
#define LSAT_META_H

#include "lsat.h"

/* Input metadata */
#define L_SENSOR	  	"SENSOR"
#define L_SCENEID		"LANDSAT_SCENE_ID"
#define L_PRODUCTID		"LANDSAT_PRODUCT_ID"	 /* New in collection-based */
#define L_DATA_TYPE  		"DATA_TYPE"		 /* Applied both pre-collection and collection */
#define L_TILE_ID   		"TILE_ID"
#define L_SENSING_TIME   	"SENSING_TIME"
#define L_L1PROCTIME     	"L1_PROCESSING_TIME"	/* Use ARCHIVING_TIME for L1PROCTIME */
#define L_USGS_SOFTWARE		"USGS_SOFTWARE"
#define L_TIRS_SSM_MODEL  	"TIRS_SSM_MODEL"
#define L_TIRS_SSM_POSITION_STATUS 	"TIRS_SSM_POSITION_STATUS"

#ifndef L_HORIZONTAL_CS_NAME 	/* They were originally defined here but moved to lsat.h */
#define L_HORIZONTAL_CS_NAME 	"HORIZONTAL_CS_NAME"
#define L_ULX  "ULX"
#define L_ULY  "ULY"
#endif

#define L_NROWS  "NROWS"
#define L_NCOLS  "NCOLS"
#define L_SPATIAL_RESOLUTION "SPATIAL_RESOLUTION"
#define L_MSZ  "MEAN_SUN_ZENITH_ANGLE"
#define L_MSA  "MEAN_SUN_AZIMUTH_ANGLE"
#define L_MVZ  "MEAN_VIEW_ZENITH_ANGLE"
#define L_MVA  "MEAN_VIEW_AZIMUTH_ANGLE"

/* AROP summary */
#define L_AROP_BASE_SENTINEL2 	"arop_s2_refimg"
#define L_AROP_NCP 		"arop_ncp"
#define L_AROP_RMSE 		"arop_rmse(meters)"
#define L_AROP_XSHIFT 	"arop_ave_xshift(meters)"
#define L_AROP_YSHIFT 	"arop_ave_yshift(meters)"
/* other */
#define L_SPCOVER  "spatial_coverage"
#define L_CLCOVER  "cloud_coverage"

#define L_ACCODE "ACCODE"	/* atmospheric correction code.  Nov 21, 2017 */
#define L_HLSTIME "HLS_PROCESSING_TIME"	      /* Set directly in C code after processing??? 11/21/17 */

typedef struct {
	int nscene; 	/* Two scenes of the same day,  same path and adjacent rows can fall
			 * on a tile.  This variable indicates how many for those 
			 * numerical arrays; for the characters arrays, the metadata for
			 * multiple scenes will be concatenated. */ 
	char sensor[100];
	char sceneid[200];
	char productid[200];	/* new for collection-based */
	char datatype[100];
	char granuleid[10];   /* e.g. 11SKA */
	char sensing_time[300];
	char software[100];	/* e.g. LPGS_2.6.2 */
	char l1proctime[300];
	char cs_name[200];
	int  nr;  	/* no. of rows */
	int  nc;  	/* no. of columns*/
	float64 spatial_resolution;  
	float64 ululx;	/* upper-left corner of the upper-left pixel. It is for a scene or a tile */
	float64 ululy;
	char ssm_model[200];
	char ssm_position[200];
	float64 msz[2];	 /* Up to two scenes on a tile */
	float64 msa[2];
	float64 mvz;	/* not avail */
	float64 mva;	/* not avail */

	/* AROP */
	char s2basename[300]; 
	int ncp[2];		
	double rmse[2];
	double addtox[2]; 
	double addtoy[2]; 

	/* Other */
	int16 spcover;		/* Spatial coverage in percentage */
	int16 clcover; 		/* Cloud coverage in percentage */
	char accode[100]; 	/* atmospheric correction code name */
	char hlstime[300];
} lsatmeta_t;

int write_input_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta);
int set_input_metadata(lsat_t *lsat, char *mtl);
int get_input_metadata(lsat_t *lsat, lsatmeta_t *lsatmeta);

/* Copy some of the input scene  metadata during the tiling of a landsat scene. But use the tile's ULX and dimension info  */
int copy_input_metadata_to_tile(lsat_t *lsat, lsatmeta_t *lsatmeta);

/* When two scenes of the same day fall on the same tile (i.e., same path, adjacent row, same day),
 * update the input metadate by concatenating. 
 */
int update_input_metadata_for_tile(lsat_t *lsat, lsatmeta_t *lsatmeta);


/* When there is only one scene on the tile so far */
int set_arop_metadata(lsat_t *lsat,  
			char *s2basename, 
			int ncp, 
			double rmse, 
			double shiftx, 
			double shifty);



/* Update AROP metadata when the second scene comes in */
int update_arop_metadata(lsat_t *lsat, 
			char *s2basename, 
			int ncp, 
			double rmse, 
			double shiftx, 
			double shifty);
#endif
