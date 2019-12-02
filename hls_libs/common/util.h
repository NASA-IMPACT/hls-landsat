#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mfhdf.h"

/**************************************************************************/
/* Return 1 if file exists, 0 otherwise. */
int file_exist(char *fname);


/**************************************************************************/
/* Add current time as processing time to each product */
int getcurrenttime(char *timestr);


/**************************************************************************/
/* Name ERROR is also used by GCTP, so start to use Error instead.*/
#define MSGLEN 5000
#define Error(message) \
          _Error_((message), (__FUNCTION__), (__FILE__), (long)(__LINE__))

void _Error_(const char *message, const char *module, 
           const char *source, long line);



/* Convert a double number to a int16 type */
int16 asInt16(double tmp);

/**************************************************************************/
/* Add an ENVI header for ENVI users. HLS processing does not need the header */
int add_envi_utm_header(char *zonehem, 
			double ulx, 
			double uly, 
			int  nrow, 
			int ncol, 
			double pixsz,
			char *fname_hdr);

/* Still used in tiling the L8 geometry file, since geolocation information was
 * not written to the scene-based geometry hdf file.
 */
int read_envi_utm_header(char *fname, 
			char *zonehem, 
			double *ulx, 
			double *uly);

#endif
