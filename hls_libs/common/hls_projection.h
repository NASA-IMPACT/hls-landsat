#ifndef HLS_PROJECTION_H
#define HLS_PROJECTION_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <ctype.h>
#include "cproj.h"
#include "proj.h"

// #include "util.h"
// #include "hls_commondef.h"

/*******************************************************************************
NAME:     utm2lonlat()
*******************************************************************************/
/* For UTM, GCTP does not use a false northing for Y, but the standard false
 * northing for the southern hemisphere is 10,000,000 meters. Note that for the
 * southern hemisphere, the input Y has been converted according to GCTP style.
 * Oct 5, 2015. 
 */
void utm2lonlat(int utmzone, double ux, double uy, double *lon, double *lat);
void lonlat2utm(int utmzone, double lon, double lat, double *ux, double *uy);

/* Sometimes data near the UTM zone boundary needs to be reprojected from one zone to
 * the adjacent zone. Note that for the southern hemisphere, the input Y has been 
 * converted according to GCTP style.
 */
void utm2utm(int inzone, double inx, double iny, int outzone, double *outx, double *outy);

/* Sinusoidal to geographic via direct inversion */
/* Return 0 for valid retrieval, non-zero for out of bound due to invalid input X. */
int sin2geo(double sx, double sy, double *lon, double *lat);

/* Sinusoidal to geographic via gctp */
void sin2geo_gctp(double sx, double sy, double *lon, double *lat);

/* Sinusoidal to UTM */
void sin2utm(double sx, double sy, int utmzone, double *ux, double *uy); 

/* UTM to sinusoidal */
void utm2sin(int utmzone, double ux, double uy, double *sx, double *sy); 

#endif
