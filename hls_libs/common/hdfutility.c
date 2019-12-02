#include "hdfutility.h"

int PutSDSDimInfo(int32 sds_id, char *dimname, int irank)
{
	int32 dimid;
	char message[MSGLEN];

	if ((dimid = SDgetdimid(sds_id, irank)) == FAIL) {
    		sprintf(message, "Error in SDgetdimid() for rank %d", irank);
    		Error(message);
		return(ERR_CREATE);
	}

	if (SDsetdimname(dimid, dimname) == FAIL) {
    		sprintf(message, "Error in SDsetdimname() for rank %d", irank);
    		Error(message);
		return(ERR_CREATE);
	}

	return(0);
}
