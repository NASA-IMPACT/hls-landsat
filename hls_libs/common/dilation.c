#include "dilation.h"


/* Dilate certain mask values by a number of pixels. 
 * The values to be dilated (seeds) are defined in isSeed().
 *   mask: cloud mask array
 *   nrow: 
 *   ncol: 
 *   hw: half size of the dilation window (kernel as people say?)
 *
 */
void dilate(unsigned char *mask, int nrow, int ncol, int hw) 
{
	unsigned char *dm;		/* temporarily to hold the dilated mask */
	unsigned char dval = 254; 	/* Label in the dilation. */
	unsigned short *dis; 		/* squared distance between a target pixel and the nearest seed pixel.*/
	unsigned short MAXDIS = 10000;	/* For initialization of dis[] */
	unsigned short d;

	/* Implementation note: Originally array dis was defined as unsigned char *, which is 
	 * big enough to hold a squared distance for 11 pixels in both X and Y direction. It 
	 * really would not make sense to dilate more than that, but changed to short type anyway.  
	 */
	
	int irow, icol, rowbeg, rowend, colbeg, colend;
	int i, j, k, n;

	if ((dm = (unsigned char*) calloc(nrow * ncol, sizeof(char))) == NULL) {
		fprintf(stderr, "Cannot allocate memory for dm\n");
		exit(1);
	}
	if ((dis = (unsigned short *) calloc(nrow * ncol, sizeof(unsigned short))) == NULL) {
		fprintf(stderr, "Cannot allocate memory for dis\n");
		exit(1);
	}

	memcpy(dm, mask, nrow * ncol);

	// memset(dis, MAXDIS, nrow * ncol);
	for (irow = 0; irow < nrow; irow++) 
		for (icol = 0; icol < ncol; icol++) 
			dis[irow*ncol+icol] = MAXDIS;

	for (irow = 0; irow < nrow; irow++) {
		for (icol = 0; icol < ncol; icol++) {
			k = irow * ncol + icol;
		      	if (isSeed(mask[k])) {
				rowbeg = irow-hw < 0 ? 0: irow-hw;
				rowend = irow+hw > nrow-1 ? nrow-1: irow+hw;
				colbeg = icol-hw < 0 ? 0: icol-hw;
				colend = icol+hw > ncol-1 ? ncol-1: icol+hw;
				
				for (i = rowbeg; i <= rowend; i++) {
					for (j = colbeg; j <= colend; j++) {
						n = i * ncol + j;
 						/* Do not dilate into nodata pixels or other seed pixels, but it is okay to 
						 * dilate into non-seed pixels because the masking may not correct there anyway.
						 */
						if (mask[n] == HLS_MASK_FILLVAL || isSeed(mask[n]))
							continue;

						d = (i-irow)*(i-irow) + (j-icol)*(j-icol);  /* Avoid sqrt for speed */
						if (d < dis[n]) {
							dm[n] = dval;
							dis[n] = d;
						}
					}	
				}
			}
		}
	}

	memcpy(mask, dm, nrow * ncol);
	free(dm);
	free(dis);
}
