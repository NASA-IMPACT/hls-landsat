#ifndef L8ANG_H
#define L8ANG_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"
#include "hls_commondef.h"
#include "hdfutility.h"
#include "util.h"
#include "fillval.h"
#include "lsat.h"    /* only for constant L8NRB */

#ifndef L1TSCENEID
#define L1TSCENEID "L1T_SceneID"
#endif

#define SZ "solar_zenith"
#define SA "solar_azimuth"
#define VZ "view_zenith"	/* VZ and VA are part of the SDS name for a band. */
#define VA "view_azimuth"

/* Pre-collection angle SDS name, from Claverie code */
static char *L8_ANG_SDS_NAME_PC[3] = { "SZA", "VZA", "RAA" };
/* Collection data angle in plain binary file (no SDS ) */

/* SDS name for the tiled angle */ 
static char *L8_SZ = "solar_zenith";
static char *L8_SA = "solar_azimuth";
static char *L8_VZ[] = {"band01_vz",
			"band02_vz",
			"band03_vz",
			"band04_vz",
			"band05_vz",
			"band06_vz",
			"band07_vz"};

static char *L8_VA[] = {"band01_va",
			"band02_va",
			"band03_va",
			"band04_va",
			"band05_va",
			"band06_va",
			"band07_va"};

typedef struct {
	char fname[NAMELEN];
	intn access_mode;
	double ulx;
	double uly;	
	char numhem[20]; 	/* UTM zone number and hemisphere spec */
	int nrow;
	int ncol;
	char l1tsceneid[200];	/* The L1T scene ID for an S2 tile */

	int32 sd_id;
	int32 sds_id_sz;
	int32 sds_id_sa;
	int32 sds_id_vz[L8NRB-1]; /* No cirrus */
	int32 sds_id_va[L8NRB-1]; 

	int16 *sz;
	int16 *sa;
	int16 *vz[L8NRB-1];	/* No cirrus band; constant L8NRB does not inlcude pan band */
	int16 *va[L8NRB-1];

	char tile_has_data; /* A scene and a tile may overlap so little that there is no data at all */  
} l8ang_t;

/* open L8 angle for READ, CREATE, or WRITE.
 * Modification on May 26, 2017.
 * The L8 view angle for a scene in a path/row is created in two formats. For the 
 * pre-collection data, Martin Claverie's TLE treats the angle of all bands the same,
 * and there is no way of taking into account the staggered layout of the CCD 
 * detector modules; the output is in hdf. For the collection data, USGS provides a 
 * tool to compute per-band geometry and the output is in TIFF. So this particular 
 * function is used to read the two types of angle input. 
 */
int read_l8ang_inpathrow(l8ang_t  *l8ang, char *fname);

/* open tile-based L8 angle for READ, CREATE, or WRITE. */
int open_l8ang(l8ang_t  *l8ang, intn access_mode); 

/* Add sceneID as an attribute. A sceneID is added to the angle output HDF when it is
 * first created.  And a second scene can fall on the scame S2 tile too, and this function
 * adds the sceneID of the second scene.
 * lsat.h has a similar function.
 */
int l8ang_add_l1tsceneid(l8ang_t *l8ang, char *l1tsceneid);

/* close */
int close_l8ang(l8ang_t  *l8ang);

#endif
