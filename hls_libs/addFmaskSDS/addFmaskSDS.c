/* Purpose:
 * 1. Add Fmask as an SDS, and dilate cloud and cloud shadow by 5 pixels.
 * 2. Incorporate the 2 bits of aerosol level into bits 6-7 of Fmask.
 * 3. Fortran LaSRC creates a CLOUD band and also passes through the
 *    L1T bandQA. Rename the first as ACmask and ignore the second. It is done in 
 *    lsat.c which is called by this code.
 * 4. Add metadata from MTL.
 * 5. Change the USGS LaSRC reflectance scaling factor and offset to the HLS 10000 and 0 respectively.
 * 6. Change the USGS LaSRC temperature scaling factor and offset to the HLS 100 and 0 respectively.
 * 7. Change temperature from Kelvin to Celsius.   
 * 8. Change the USGS fillvalue 0 to HLS -9999.
 * 9. The most challenging  part is: change the USGS uint16 to int16 numbers. 
 *    The HLS function is created to read the Fortran LaSRC output of int16, but the C LaSRC  output is
 *    uint16. The trick is: When uint16 data is read into int16 variable,  overflow may happen but the
 *    original bits are preserved; so simply cast the number back to uint16 one by one.
 *
 * Note: We create a new HDF instread of updating the AC output because we 
 *       change the SDS names to HLS convention.
 *
 * Oct 27, 2020. Revised for the new USGS LaSRC.
 * Apr 12, 2021. Incorporate the 2 bits of aerosol level into Fmask bits 6-7.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lsat.h"
#include "lsatmeta.h"
#include "hls_commondef.h"
#include "error.h"
#include "dilation.h"

/* Rename CLOUD as ACmask and reshuffle its bits, add Fmask and incorporate the aerosol level bits into Fmask */
int copyref_addmask(lsat_t *lsatin, char *fname_fmask, char *fname_aeroQA, lsat_t *lsatout);

int main(int argc, char *argv[])
{
	char fname_in[LINELEN];		/* AC output intact. */
	char fname_fmask[LINELEN];	/* Fmask plain binary */
	char fname_aeroQA[LINELEN];	/* Aerosol QA from USGS LaSRC*/
	char fname_mtl[LINELEN];	/* mtl */
	char accodename[LINELEN];	/* Atmospheric correction code name; to be added as hdf attribute */
	char fname_out[LINELEN];	/* Output */

	lsat_t lsatin, lsatout;
	char creationtime[100];

	int ret;

	if (argc != 7) {
		fprintf(stderr, "%s lsat_in fmask aeroQA mtl accodename lsat_out\n", argv[0]);
		exit(1);
	}

	strcpy(fname_in,    argv[1]);
	strcpy(fname_fmask, argv[2]);
	strcpy(fname_aeroQA, argv[3]);
	strcpy(fname_mtl,   argv[4]);
	strcpy(accodename,  argv[5]);
	strcpy(fname_out,   argv[6]);

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
	copyref_addmask(&lsatin, fname_fmask, fname_aeroQA, &lsatout);

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


/* Rename CLOUD as ACmask and reshuffle its bits, add Fmask and incorporate the aerosol level bits into Fmask.
 * Change the original uint16 to int16 during scale change, change Kelvin to Celsius. */
/* Oct 27, 2020
 * Apr 12, 2021 */
int copyref_addmask(lsat_t *lsatin, char *fname_fmask, char *fname_aeroQA, lsat_t *lsatout)
{	
	int ib, k, npix;
	unsigned char mask, val;
	char message[MSGLEN];

	/* Used to change uint16 to int16 in scale conversion */
	uint16 us16;
	int16  s16;

	/* Spectral bands */
	/* Reflectance */
	npix = lsatin->nrow * lsatin->ncol;
	for (ib = 0; ib < L8NRB; ib++) {
		for (k = 0; k < npix; k++) {
			/* Bug. If overflows occurs for int16 in lsat.c, numbers become negative */
			/* if (lsatin->ref[ib][k] > 0) { */
			if (lsatin->ref[ib][k] != 0) { 
				memcpy(&us16, &lsatin->ref[ib][k], 2);
				s16 = asInt16((us16 * 2.75E-5 - 0.2) * 1E4);
				lsatout->ref[ib][k] = s16;
			}
		}
	}
	/* Thermal. Change to Celsius */
	for (ib = 0; ib < 2; ib++) {
		for (k = 0; k < npix; k++) {
			//if (lsatin->thm[ib][k] > 0) {
			if (lsatin->thm[ib][k] != 0) {
				memcpy(&us16, &lsatin->thm[ib][k], 2);
				s16 = asInt16(((us16 * 0.00341802  + 149) - 273.16) * 100);
				lsatout->thm[ib][k] = s16;
			}
		}
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
		/* Set every mask pixel to 0, essentially disabling this SDS.  Oct 27, 2020 */
		lsatin->accloud[k] =  0;

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

	/* Read the aerosol QA */
	unsigned char *aeroQA, aero;
	FILE *faeroQA;

	if ((aeroQA = (uint8*)calloc(npix, sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory\n");
		return(1);
	}
	if ((faeroQA = fopen(fname_aeroQA, "r")) == NULL) {
		sprintf(message, "Cannot read aerosol QA %s\n", fname_aeroQA);
		Error(message);
		return(1);
	}
	if (fread(aeroQA, sizeof(uint8), npix, faeroQA) !=  npix ) {
		sprintf(message, "Aerosol QA file size wrong: %s\n", fname_aeroQA);
		Error(message);
		return(1);
	}
	fclose(faeroQA);

	/* Dilate by 5 pixels (i.e., 150m. Effectively the same on the Sentinel-2 side, 7 pixels of 20m). */
	dilate(fmask, lsatin->nrow, lsatin->ncol, 5);

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
			case 254: 	/* Dilated cloud or cloud shadow. Now becomes "adjacent to cloud" */
				val = 1;
				mask = (val << 2);
				break;

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

		/* Aerosol level */
		aero = (aeroQA[k] >> 6) & 03;
		mask = ((aero << 6) | mask);

		/* final */
		lsatout->fmask[k] = mask;
	}

	return(0);
}
