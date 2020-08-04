#ifndef L8SR_H
#define L8SR_H

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"
#include "util.h"
#include "hdfutility.h"
#include "hls_commondef.h"
#include "fillval.h"

#define L8NRB 	 8 	 	/* reflective bands, no pan */
#define L8NTB 	 2 	 	/* thermal bands */

/* The first group of SDS names are used by Eric Vermote, who
 * added the visible color names.  HLS uses the second group.
 */
static char *L8_REF_SDS_NAME[][L8NRB] = {      {"band01",
						"band02-blue",
						"band03-green",
						"band04-red",
						"band05",
						"band06",
						"band07",
						"band09"},
	
					       {"B01",
					  	"B02",
						"B03",
						"B04",
						"B05",
						"B06",
						"B07",
						"B09"}
				         };

static char *L8_THM_SDS_NAME[][2] = { 	{"band10", 
					 "band11"},

					{"B10",
					 "B11"}
				    };
					
static char *L8_BANDQA_SDS_NAME = "bandQA";	/* Carryover from TOA input, available in AC output*/
static char *L8_CLOUD_SDS_NAME = "CLOUD";	/* Created in atmospheric correction */
//static char *L8_QA_SDS_NAME = "QA";		/* Combination of the above two for HLS*/
static char *L8_ACMASK_SDS_NAME = "ACmask";		/* Renamed from CLOUD but with bits reshuffled. */ 
static char *L8_FMASK_SDS_NAME = "Fmask";		/* Fmask4.0 */

/* Long names of SDS for HLS */
static char *L8_REF_SDS_LONG_NAME[] = { "Coastal_Aerosol",
					"Blue",
					"Green",
					"Red",
					"NIR",
					"SWIR1",
					"SWIR2",
					"Cirrus"
				       };
static char *L8_THM_SDS_LONG_NAME[] = { "TIRS1", 
					"TIRS2"};
static char *L8_QA_SDS_LONG_NAME = "Bit_packed_QA";		/* Combination of the above two*/

/* A flag for whether combine bandQA and CLOUD, right after atmospheric correction. */
#define COMBINEQA "combine_bandQA_and_CLOUD"

/* Output from Vermote code does not have the two HLS mask SDS, but a CLOUD SDS.
 * The default is to ignore AC CLOUD but use ACmask and Fmask.
 */
#define AC_CLOUD_AVAILABLE  "ac_cloud_available"
#define MASK_UNAVAILABLE    "mask_unavailable"

/* The upperleft X, Y coordinates as HDF attributes are not set yet in the output of the
 * LDCM AC code (whose HDFEOS coordinates are written erroneously). The X/Y coordinates
 * will be set in the combineQA code by reading MTL and available for HLS.
 *
 * L_ULX, L_ULY, and L_HORIZONTAL_CS_NAME were originally defined in "lsatmeta.h". But 
 * they are needed in lsat.c to read these attributes if they are available.
 */
#define ULXYNOTSET "upperleft_xy_not_set"  
#define L_ULX  "ULX"
#define L_ULY  "ULY"
#define L_HORIZONTAL_CS_NAME 	"HORIZONTAL_CS_NAME"
#define INWRSPATHROW "DATA_IN_WRS_PATH_ROW"
#define L_S2TILEID "SENTINEL2_TILEID"
#define SPCOVER  "spatial_coverage"
#define CLCOVER  "cloud_coverage"

typedef struct {
	char fname[LINELEN];
	intn access_mode;
	int nrow;
	int ncol;
	char zonehem[10];	/* UTM zone and hemisphere: e.g. 10S */
	double ulx;
	double uly;

	char ac_cloud_available[100]; 	/* Used to indicate whether it is the file directly from AC.
				         * Much safer than using an integer indicator. 
					 */ 
	/* Not needed because it is the compliment of ac_cloud_available. Aug 8, 2019 */
        //char mask_unavailable[100]; /* ACmask and Fmask not available yet, i.e. this is output from AC code */

	char ulxynotset[30]; 	
	char inpathrow[30];	/* data in Path/Row, before regridding into S2 tiles */

	int32 sd_id;
	int32 sds_id_ref[L8NRB];
	int32 sds_id_thm[L8NTB];
	int32 sds_id_accloud;     /* Only used in read CLOUD SDS from AC. Used no more subsequently. */
	int32 sds_id_acmask;	/* Renamed from CLOUD and with bits reshuffled */
	int32 sds_id_fmask;	/* Fmask*/

	int16 *ref[L8NRB];
	int16 *thm[L8NTB];
	//uint16 *bandqa;		/* from L1T, but won't be retained */ 
	uint8  *accloud;		/* from AC */
	uint8  *acmask;		/* Renamed from CLOUD and with bits reshuffled */
	uint8  *fmask;

	char tile_has_data;	/* Indicate whether a created tile has data; alway true for a scene */

	/* For BRDF-adjusted reflectance */
	char is_nbar[100]; 
	int32 sds_id_brdfflag;	
	uint8 *brdfflag; 

	/* AROP summary, for each L1T Landsat scene falls on a Sentinel-2 tile. No more than 4 scenes per tile (?)*/
	char l1tsceneid[1000];	/* L1T scene ID on a tile. Space-separated concatenated string */
	char s2basename[1000];	/* Basename of S2 used for each L1T scene in AROP */	
	int32 ncp[4];		/* Number of control points for each L8-S2 pair in AROP run */ 
	double rmse[4];
	double addtox[4];
	double addtoy[4];

	/* Spatial and clod cover */
	int16 spcover;
	int16 clcover; 

} lsat_t;		/* Landsat surface reflectance + thermal bands */

/* Content of a L8 pixel. Might use in cubic convolution */
/* Aug 8, 2019. Yes, used in sr_tile  */
typedef struct {
	int16 ref[L8NRB];
	int16 thm[L8NTB];
	uint8 acmask;
	uint8 fmask;
} lsatpix_t;		

/* open sr for DFACC_READ or DFACC_CREATE, or DFACC_WRITE.
 */
int open_lsat(lsat_t *lsat, intn access_mode); 

/* Combine  bandQA and CLOUD sds into one, right after atmospheric correction. */
int combine_qa(lsat_t *lsat);

/* Add L1T sceneID as an attribute. A sceneID can be added for the first time for an HDF;
 * a second scene can fall on a S2 tile too. Used in rendering to S2 tiles.
 */
/* Jul 20, 2016: No longer needed?? */
int add_l1tsceneid(lsat_t *lsat, char *l1tsceneid);

/* Write some AROP metadata. After L8 is rendered into S2 tile */
/* Jul 20, 2016: No longer needed?? */
int write_arop_attri(lsat_t *lsat, char *s2base, int ncp, double rmse, double shiftx, double shifty);

/* Solar zenith angle in reflectance normalization */
int write_solarzenith_lsat(lsat_t *lsat, double sz);

/* close sr */
int close_lsat(lsat_t *lsat);

/* copy one hdf to another. Used in BRDF adjustment.
 * July 20, 2016: No longer needed???
*/
// void copy_input(lsat_t *out, lsat_t *in);
// void dup_input(lsat_t *in, lsat_t *out);

/* Spatial and cloud coverage */
void lsat_setcoverage(lsat_t *lsat);

#endif
