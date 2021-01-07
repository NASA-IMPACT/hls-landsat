#ifndef HLS_COMMONDEF_H
#define HLS_COMMONDEF_H


/* BRDF-adjustment can make huge mistakes. Constrain the resulting values */
#define HLS_INT16_MIN -32768
#define HLS_INT16_MAX  32767

/* Dec 3, 2020: These strings are are used as metadata for SDS */
static char *L8_ref_fillval = "-9999";
static char *L8_ref_scale_factor = "0.0001";
static char *L8_ref_add_offset = "0.0";

static char *L8_thm_fillval = "-9999";
static char *L8_thm_scale_factor = "0.01";
static char *L8_thm_add_offset = "0.0";
static char *L8_thm_unit = "Celsius";

static char *S2_ref_fillval = "-9999";
static char *S2_ref_scale_factor = "0.0001";
static char *S2_ref_add_offset = "0.0";

#define ACMASK_NAME "ACmask"
#define FMASK_NAME "Fmask"


#define BRDFFLAG_NAME "brdfflag"

#define L8_AC_OUT_SR_FILL (-9999) 

#define NAMELEN 500  /* obsolete? */
#define LINELEN 500

#define ERR_READ    1 
#define ERR_CREATE  2 
#define ERR_MEM     3 

#define LS_PIXSZ 30.0
#define HLS_PIXSZ 30.0 
#define HLS_TILEDIM_30M 3660	/* Number of 30m pixels */

#define S2TILESZ 109800   /* meter */

/* For both S2 and L8*/
#define IS_NBAR  "IS_NBAR"

#endif
