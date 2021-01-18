/* Purpose:
 * 1. LaSRC creates a CLOUD band and also passes through the
 *    L1T bandQA. Rename the first as ACmask and igore the second. This is done in lsat.c
 * 2. Add Fmask
 * 3. Add metadata from MTL.
 * 4. Change temperature from Kelvin to Celsius.   Jul 24, 2016.
 *
 * Note: We create a new HDF instread of updating the AC output because we 
 *       change the SDS names to HLS convention.
 *
 * A very simple piece of code. But in the future, cloud mask from other tools 
 * such as Fmask can be added and combined in this code too. 
 * (Yes, finally.  Aug 7, 2019. )
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lsat.h"
#include "lsatmeta.h"
#include "hls_commondef.h"
#include "error.h"

/* Rename CLOUD as ACmask and reshuffle its bits, add Fmask */
int copyref_addmask(lsat_t *lsatin, char *fname_fmask, lsat_t *lsstout);

int main(int argc, char *argv[])
{
	char fname_in[LINELEN];		/* AC output intact. */
	char fname_fmask[LINELEN];	/* Fmask plain binary */
	char fname_mtl[LINELEN];	/* mtl */
	char accodename[LINELEN];	/* Atmospheric correction code name; to be added as hdf attribute */
	char fname_out[LINELEN];	/* Output */

	lsat_t lsatin, lsatout;
	char creationtime[100];

	int ret;

	if (argc != 6) {
		fprintf(stderr, "%s lsat_in fmask mtl accodename lsat_out\n", argv[0]);
		exit(1);
	}

	strcpy(fname_in,    argv[1]);
	strcpy(fname_fmask, argv[2]);
	strcpy(fname_mtl,   argv[3]);
	strcpy(accodename,  argv[4]);
	strcpy(fname_out,   argv[5]);

	/*** Read the input ***/
	strcpy(lsatin.fname, fname_in);
	strcpy(lsatin.ac_cloud_available, AC_CLOUD_AVAILABLE);
	strcpy(lsatin.ulxynotset, ULXYNOTSET); 		/* UL not available from LaSRC output; do not try to read */
	ret = open_lsat(&lsatin, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_lsat()");
		exit(1);
	}

	/*** Create output ***/
	strcpy(lsatout.fname, fname_out);
	lsatout.nrow = lsatin.nrow;
	lsatout.ncol = lsatin.ncol;
	ret = open_lsat(&lsatout, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_lsat()");
		exit(1);
	}
	lsatout.tile_has_data = 1; 	/* Originally lsat.h was designed to handle tiled L8 data 
					 * (some tiled file will turn out empty), not SR in scenes. 
					 * But be sure to set to 1 so that file of scenes will not be deleted. */

	/* ACmask and Fmask */ 
	copyref_addmask(&lsatin, fname_fmask, &lsatout);

	/* Add metadata from MTL; also ulx, uly and zonehem are set */
	ret = set_input_metadata(&lsatout, fname_mtl);
	if (ret != 0) {
		Error("Error in set_input_metadata");
		exit(1);
	}
	/* Additional metadata */
	/* AC code name */
	SDsetattr(lsatout.sd_id, "ACCODE", DFNT_CHAR8, strlen(accodename), (VOIDP)accodename);
	/* time */
	getcurrenttime(creationtime);
	SDsetattr(lsatout.sd_id, "HLS_PROCESSING_TIME", DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* Change Kelvin to Celsius */
	int iband, irow, icol, k;
	for (iband = 0; iband < L8NTB; iband++) {
		for (irow = 0; irow < lsatout.nrow; irow++) {
			for (icol = 0; icol < lsatout.ncol; icol++) {
				k = irow * lsatout.ncol + icol;
				if (lsatout.thm[iband][k] == 0) /* Fill for Kelvin */
					lsatout.thm[iband][k] = HLS_THM_FILLVAL;
				else
					/* The kelvin scale factor is 10; the new Celsius is 100. */
					lsatout.thm[iband][k] = (lsatout.thm[iband][k] * 0.1 - 273.16) * 100;
			}
		}
	}  

	ret = close_lsat(&lsatin);
	if (ret != 0) {
		Error("Error in close_lsat");
		exit(1);
	}
	ret = close_lsat(&lsatout);
	if (ret != 0) {
		Error("Error in close_lsat");
		exit(1);
	}

	return 0;
}


int copyref_addmask(lsat_t *lsatin, char *fname_fmask, lsat_t *lsatout)
{	
	int ib, k, npix;
	unsigned char mask, val;
	char message[MSGLEN];

	/* Spectral bands */
	npix = lsatin->nrow * lsatin->ncol;
	for (ib = 0; ib < L8NRB; ib++) {
		for (k = 0; k < npix; k++)
			lsatout->ref[ib][k] = lsatin->ref[ib][k];
	}
	for (ib = 0; ib < 2; ib++) {
		for (k = 0; k < npix; k++)
			lsatout->thm[ib][k] = lsatin->thm[ib][k];
	}

// Aug 7, 2019. This is the bit description of the v1.4 QA SDS. Although we do not keep this SDS
// in v1.5, we use the same bit description in v1.5 for both ACmask and Fmask.
//
//	/* v1.4 QA SDS */
//	/* Note: For better view with hdfdump, the blanks within the string is blank space characters, not tab */
// 	sprintf(attr,   "Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
//                      "7-6    aerosol:\n"
//                      "       00 - climatology\n"
//                      "       01 - low\n"
//                      "       10 - average\n"
//                      "       11 - high\n"
//		  	"5      water\n"
//			"4      snow/ice\n"
//			"3      cloud shadow\n"
//    			"2      adjacent to cloud; \n",
//			"1      cloud\n"
//			"0      cirrus\n");
//	SDsetattr(s2r->sds_id_qa, "QA description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);
//
//	/* The original CLOUD SDS from LaSRC 
//	CLOUD:QA index = "\n",
//	    		"\tBits are listed from the MSB (bit 7) to the LSB (bit 0):\n",
//	    		"\t7      internal test; \n",
//	    		"\t6      unused; \n",
//	    		"\t4-5     aerosol;\n",
//	    		"\t       00 -- climatology\n",
//	    		"\t       01 -- low\n",
//	    		"\t       10 -- average\n",
//	    		"\t       11 -- high\n",
//	    		"\t3      cloud shadow; \n",
//	    		"\t2      adjacent to cloud; \n",
//	    		"\t1      cloud; \n",
//	    		"\t0      cirrus cloud; \n",
//	*/

	/* v1.5 ACmask. Essentially CLOUD but with some bits swapped in compliance with the v1.4 QA SDS  */
	for (k = 0; k < npix; k++) { 
		if (lsatin->accloud[k] == HLS_MASK_FILLVAL)
			continue;

		mask = 0;

		/* Bits 0-3 are the same for CLOUD and ACmask.
		 * They are: CIRRUS, cloud, adjacent to cloud, and cloud shadow.
		 */
		mask = lsatin->accloud[k] & 017; 	/* 017 is binary 1111 */

		/* Reserve bit 4 for snow/ice, which is not set in AC CLOUD  */

		/* Reserve bit 5 for water, which is not set in AC CLOUD. Although bit 7 of AC CLOUD
		 * is "internal test" for water (Oct 3, 2017), the quality is very poor; do not use.  Jun 27, 2019 
		 */

		/* 2-bit aerosol level, shift from original CLOUD bits 4-5 to ACmask bits 6-7 */
		mask = mask | (((lsatin->accloud[k] >> 4 ) & 03) << 6);

		/* Finally */
		lsatout->acmask[k] = mask;
	}


	/***** Fmask *****/

	unsigned char *fmask;
	FILE *ffmask;

	if ((fmask = (uint8*)calloc(npix, sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory\n");
		return(1);
	}
	if ((ffmask = fopen(fname_fmask, "r")) == NULL) {
		sprintf(message, "Cannot read Fmask %s\n", fname_fmask);
		Error(message);
		return(1);
	}
	if (fread(fmask, sizeof(uint8), npix, ffmask) !=  npix ) {
		sprintf(message, "Fmask file size wrong: %s\n", fname_fmask);
		Error(message);
		return(1);
	}
	fclose(ffmask);

	for (k = 0; k < npix; k++) { 

		if (fmask[k] == HLS_MASK_FILLVAL)	/* Fmask has used 255 as fill */
			continue;

		/* fmask
		clear land = 0
		water = 1
		cloud shadow = 2
		snow = 3
		cloud = 4
		thin_cirrus = 5		#  Seems cirrus is dropped in v4.0?   Mar 20, 2019 
		*/

		switch(fmask[k])
		{
			case 5: 	/* CIRRUS. But not set in Fmask4.0. A placeholder for now. Aug 7, 2019 */
				val = 1;
				mask = val;
				break;
			case 4: 	/* cloud*/ 
				val = 1;
				mask = (val << 1);
				break;
			case 3: 	/* snow/ice. */
				val = 1;
				mask = (val << 4);
				break;
			case 2: 	/* Cloud shadow */
				val = 1;
				mask = (val << 3);
				break;
			case 1: 	/* water*/
				val = 1;
				mask = (val << 5);
				break;
			case 0:
				mask = 0;
				break;	/* clear; do nothing */
			default: 
				sprintf(message, "Fmask value not expected: %d", fmask[k]);
				Error(message);
				exit(1);
		}
		lsatout->fmask[k] = mask;
	}

	return(0);
}
