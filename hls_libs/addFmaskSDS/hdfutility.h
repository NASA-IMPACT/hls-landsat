#ifndef HDFUTILITY_H
#define HDFUTILITY_H

#include "mfhdf.h"

#include "util.h"
#include "hls_commondef.h"

/* Write SDS dimension name */

static char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
int PutSDSDimInfo(int32 sds_id, char *dimname, int irank);

#endif
