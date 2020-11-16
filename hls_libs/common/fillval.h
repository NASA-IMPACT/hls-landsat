#ifndef FILLVAL_H
#define FILLVAL_H

#include "mfhdf.h"

/* This is what are used in HLS products */
static int16 ref_fillval = -9999;
static int16 thm_fillval = -9999;
static uint8 hls_mask_fillval = 255; 	/* Added on Nov 5, 2020 */
static unsigned char S2_mask_fillval = 255;
static float cfactor_fillval = -1000;
static unsigned char brdfflag_fillval = 0;

/* This is the output directly from the AC code; will be replaced with ref_fillval in postprocessing,
 * to be consistent with Landsat.
 */
#define AC_S2_FILLVAL (-100)
#define HLS_S2_FILLVAL     (-9999)	/* Fill value for HLS surface reflectance*/
#define HLS_REFL_FILLVAL   (-9999)	/* Fill value for HLS surface reflectance*/
#define HLS_THM_FILLVAL    (-9999) 	/* Landsat thermal */
#define HLS_MASK_FILLVAL   (255) 	/* One-byte QA. Added Apr 6, 2017 */
#define AC_S2_CLOUD_FILLVAL (24)	/* Used only in intermediate steps leading to S10 */

#define ANGFILL 40000	/* Angle. This is for Sentinel-2. Angles are uint16. */
#define USGS_ANGFILL	0	/* USGS uses 0, to be changed to ANGFILL in HLS */



#endif
