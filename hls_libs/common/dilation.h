#ifndef DILATION_H
#define DILATION_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fillval.h"

/* Dilate Fmask cloud and cloud shadow. A Fmask pixel is 1 byte. */

/* Used in dilate(): whether a mask value is one of the seed values. 
 * A seed value is one of the given values and will expand into
 * neighboring pixels. But seeds won't expand into each other. 
 *
 * Return 1 for yes and 0 for no.
 *
 * This function is used for clarity and consistency only.
 */
static inline char isSeed(unsigned char val)
{
	/* Fmask values:
	clear land = 0
	water = 1
	cloud shadow = 2
	snow = 3
	cloud = 4
	*/

	return (val == 2 || val == 4);
}

/* Dilate certain mask values by a number of pixels. 
 * The values to be dilated (seeds) are defined in isSeed().
 *   mask: cloud mask array
 *   nrow: 
 *   ncol: 
 *   hw: half size of the dilation window (kernel as people say?)
 *
 * Output: the pixels in dilated area is labeled 254. Fmask is using only 0-4.
 */
void dilate(unsigned char *mask, int nrow, int ncol, int hw);

#endif 
