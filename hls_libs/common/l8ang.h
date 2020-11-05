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

/* SDS name for the tiled angle */ 
static char *L8_SZ = "solar_zenith";
static char *L8_SA = "solar_azimuth";
static char *L8_VZ = "view_zenith";
static char *L8_VA = "view_azimuth";

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
	int32 sds_id_vz;
	int32 sds_id_va; 

	/* Oct 23, 2020: Change data type from int16 to uint16 to be consistent with S2 angles.
	 * Change the USGS int16 data to uint16 during reading. */
	uint16 *sz;
	uint16 *sa;
	uint16 *vz;	
	uint16 *va;

	char tile_has_data; /* A scene and a tile may overlap so little that there is no data at all */  
} l8ang_t;

int read_l8ang_inpathrow(l8ang_t  *l8ang, char *fname);

/* Open tile-based L8 angle for READ, CREATE, or WRITE. */
int open_l8ang(l8ang_t  *l8ang, intn access_mode); 

/* Add sceneID as an HDF attribute. A sceneID is added to the angle output HDF when it is
 * first created.  And a second scene can fall on the scame S2 tile (adjacent row in the same orbit), 
 * and this function adds the sceneID of the second scene.
 * lsat.h has a similar function.
 */
int l8ang_add_l1tsceneid(l8ang_t *l8ang, char *l1tsceneid);

/* close */
int close_l8ang(l8ang_t  *l8ang);

#endif
