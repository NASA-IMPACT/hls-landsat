#ifndef FILLVAL_H
#define FILLVAL_H

#include "mfhdf.h"

/* This is what are used in HLS products */
//static int16 ref_fillval = -1000;
//static int16 thm_fillval = -10000;
// Mar 19, 2020. Match USGS
static int16 ref_fillval = -9999;
static int16 thm_fillval = -9999;

/* This is the output directly from the AC code; will be replaced with ref_fillval in postprocessing,
 * to be consistent with Landsat.
 */
#define AC_S2_FILLVAL (-100)
//#define HLS_S2_FILLVAL     (-1000)	/* Fill value for HLS surface reflectance*/
//#define HLS_REFL_FILLVAL   (-1000)	/* Fill value for HLS surface reflectance*/
//#define HLS_THM_FILLVAL    (-10000) 	/* Landsat thermal */
// Mar 19, 2020. Match USGS
#define HLS_S2_FILLVAL     (-9999)	/* Fill value for HLS surface reflectance*/
#define HLS_REFL_FILLVAL   (-9999)	/* Fill value for HLS surface reflectance*/
#define HLS_THM_FILLVAL    (-9999) 	/* Landsat thermal */
#define HLS_MASK_FILLVAL   (255) 	/* One-byte QA. Added Apr 6, 2017 */
#define AC_S2_CLOUD_FILLVAL (24)	/* Used only in intermediate steps leading to S10 */

#define ANGFILL 40000	/* Angle. This is for Sentinel-2. Angles are uint16. */

// #define LANDSAT_ANGFILL -30000	
// July 28, 2020: Fill value used by ESPA code:
// #define LANDSAT_ANGFILL -32768
// 23 Oct, 2020. Finally, consistent with Sentinel-2. So not needed at all. 
// #define LANDSAT_ANGFILL ANGFILL
#define USGS_ANGFILL	0	/* USGS uses 0, to be changed to ANGFILL in HLS */



#endif
