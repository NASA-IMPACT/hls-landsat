#include <sys/stat.h>
#include "hls_commondef.h"
#include "util.h"

int file_exist(char *fname)
{
        struct stat stbuf;
        if (stat(fname, &stbuf) == -1)
                return 0;
        else
                return 1;
}


int getcurrenttime(char *timestr)
{
	time_t createtime;
	struct tm *ftm;

	createtime = time(NULL);
	if (createtime == -1) {
        	fprintf(stderr, "Error in time(NULL)\n");
		return(-1);
	}

	ftm = gmtime(&createtime);

	sprintf(timestr, "%d-%02d-%02dT%02d:%02d:%02dZ",   ftm->tm_year + 1900,
                                                                ftm->tm_mon+1,
                                                                ftm->tm_mday,
                                                                ftm->tm_hour,
                                                                ftm->tm_min,
                                                                ftm->tm_sec);
	return(0);
}

void _Error_(const char *message, const char *module, 
           const char *source, long line)
{
	char curtime[100];
	getcurrenttime(curtime);

	fprintf(stderr, "%s: %s, in function %s, [%s: line %ld]\n", curtime, message, module, source, line);
}



int16 asInt16(double tmp)
{
	int16 ret;

	tmp = floor(tmp + 0.5);
	if (tmp > HLS_INT16_MAX)
		ret = HLS_INT16_MAX;
	else if (tmp < HLS_INT16_MIN)
		ret = HLS_INT16_MIN;
	else
		ret = tmp;

	return ret;
}

	
int add_envi_utm_header(char *zonehem, double ulx, double uly, int  nrow, int ncol, double pixsz, char *fname_hdr)
{
	char North_South[20];
	int zone;
	double central_meridian;
	double false_northing;

	FILE *fhdr;

	if ((fhdr = fopen(fname_hdr, "w")) == NULL) {
		fprintf(stderr, "Cannot create %s\n", fname_hdr);
		return(1);
	}

	if (strstr(zonehem, "N")) {
		strcpy(North_South, "North");
		false_northing = 0;
	}
	else if (strstr(zonehem, "S")) {
		strcpy(North_South, "South");
		//false_northing = 10000000;
		// Always: no false northing. Oct 1, 2018
		false_northing = 0;
	}
	else {
		fprintf(stderr, "The zonehem does not give N/S: %s", zonehem);
		return(1);
	}

	zone = atoi(zonehem);
	central_meridian = -180 + (zone * 6 - 3); 

	/* ULX, ULY refer to the upper-left corner of the first pixel */
	fprintf(fhdr, "ENVI\n");
	fprintf(fhdr, "description = {HDF Imported into ENVI.}\n");
	fprintf(fhdr, "samples = %d\n", ncol);
	fprintf(fhdr, "lines = %d\n", nrow);
	fprintf(fhdr, "bands = 1\n");
	fprintf(fhdr, "header offset = 0\n");
	fprintf(fhdr, "file type = HDF Scientific Data\n");
	fprintf(fhdr, "data type = 2\n");
	fprintf(fhdr, "interleave = bsq\n");
	fprintf(fhdr, "byte order = 0\n");
	fprintf(fhdr, "map info = {UTM, 1.0, 1.0, %lf, %lf, %lf, %lf, %d, %s, WGS-84, units=Meters}\n",
		ulx, uly, pixsz, pixsz, zone, North_South);
	fprintf(fhdr, "coordinate system string = {PROJCS[\"UTM_Zone_%s\",GEOGCS[\"GCS_WGS_1984\","
		      "DATUM[\"D_WGS_1984\",SPHEROID[\"WGS_1984\",6378137.0,298.257223563]],"
		      "PRIMEM[\"Greenwich\",0.0],UNIT[\"Degree\",0.0174532925199433]],"
		      "PROJECTION[\"Transverse_Mercator\"],PARAMETER[\"False_Easting\",500000.0],"
		      "PARAMETER[\"False_Northing\",%lf],PARAMETER[\"Central_Meridian\",%lf],"
		      "PARAMETER[\"Scale_Factor\",0.9996],PARAMETER[\"Latitude_Of_Origin\",0.0],UNIT[\"Meter\",1.0]]}\n", 
			zonehem, false_northing, central_meridian);

	return(0);
} 

int read_envi_utm_header(char *fname, char *zonehem, double *ulx, double *uly)
{
	/* Read zone, ulx, uly from header file, since they are not written in output hdf of atmospheric correction. */
	FILE *fhdr;
	char line[1000];
	char *chpos;
	int zonenum;
	char northsouth[100]; /* Make it big lest wrong input cause crash */

	if ((fhdr = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Header not found: %s\n", fname);
		return (1);
	}
	while (fgets(line, sizeof(line), fhdr)) {	
		// map info = {UTM, 1.0, 1.0, 603585.000000, 5375415.000000, 30.000000, 30.000000, 11, North, WGS-84, units=Meters}
		if (strstr(line, "map info") != NULL  && strstr(line, "UTM") != NULL) {
			chpos = strtok(line, ",");	/* chpos points to the string preceding the first comma */
			chpos = strtok(NULL, ",");
			chpos = strtok(NULL, ",");
			chpos = strtok(NULL, ",");
			sscanf(chpos, "%lf", ulx);
			chpos = strtok(NULL, ",");
			sscanf(chpos, "%lf", uly);
			chpos = strtok(NULL, ",");
			chpos = strtok(NULL, ",");
			chpos = strtok(NULL, ",");
			sscanf(chpos, "%d", &zonenum);
			chpos = strtok(NULL, ",");
			sscanf(chpos, "%s", northsouth);

			if (strcmp(northsouth, "South") == 0 || strcmp(northsouth, "south") == 0) 
				sprintf(zonehem, "%dS", zonenum);
			else if (strcmp(northsouth, "North") == 0 || strcmp(northsouth, "north") == 0) 
				sprintf(zonehem, "%dN", zonenum);
			else {
				fprintf(stderr, "UTM N/S not specified in %s\n", fname);
				exit(1);
			}

			break;
		}
	}
	fclose(fhdr);

	return 0;
}
