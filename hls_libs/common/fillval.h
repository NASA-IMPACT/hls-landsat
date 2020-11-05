#ifndef FILLVAL_H
#define FILLVAL_H

#include "mfhdf.h"

/* This is what are used in HLS products */
static int16 ref_fillval = -1000;
static int16 thm_fillval = -10000;

/* This is the output directly from the AC code; will be replaced with ref_fillval in postprocessing,
 * to be consistent with Landsat.
 */
#define AC_S2_FILLVAL (-100)
#define HLS_S2_FILLVAL     (-1000)	/* Fill value for HLS surface reflectance*/
#define HLS_REFL_FILLVAL   (-1000)	/* Fill value for HLS surface reflectance*/
#define HLS_THM_FILLVAL    (-10000) 	/* Landsat thermal */
#define HLS_MASK_FILLVAL   (255) 	/* One-byte QA. Added Apr 6, 2017 */
#define AC_S2_CLOUD_FILLVAL (24)	/* Used only in intermediate steps leading to S10 */

#define ANGFILL 40000	/* Angle. This is for Sentinel-2. Angles are unsigned int16. 
			 * Originally thought applicable for Landsat too, but see blow */
#define LANDSAT_ANGFILL -30000	/* Martin's Matlab code saves angles as signed int16, as
				 * it saves relative azimuth rather than two separate 
				 * azimuth angles.  Although the code computes angle for every
				 * pixel, it often fails. As a result,  then the tiled geometry 
				 * will have fill values. Relative azimuth should be within
				 * [-180, 180].
				 * Retrofitting the code. Apr 5, 2017.
				 * Will abandon Matlab eventually.
				 *
				 * Apr 17, 2017: Collection-1 Landsat geometry is also signed int16.
				 */



#endif
