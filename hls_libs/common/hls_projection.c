#include "hls_projection.h"


/*******************************************************************************
NAME:     utm2lonlat()
*******************************************************************************/
void utm2lonlat(int utmzone, double ux, double uy, double *lon, double *lat)
{
	/* INTPUT PROJ PARAMS */
	double incoor[2];      //input UTM x, y
	long insys = 1;        //UTM
	long inzone;           //UTM ZONE number. 
	double inparm[15];     //parameters for UTM
	long inunit = 2;       //meters
	long indatum = 12;     //WGS 84

	/* OUTUT PROJ PARAMS */
	double outcoor[2];       //output lon/lat
	long outsys = 0;         //geographic
	long outzone = -1;       //Ignored for 
	double outparm[15];      //parameters 
	long outunit = 4;        //degrees of arc
	long outdatum = 12;      //WGS 84

	int k;

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */

	/*Projection parameter redirection*/
	long jpr = -1;                       /*don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   /*file to print the returned projection parameters*/

	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */


	/*Initialising parameters of both projections all to zeros first. No further initialized for UTM.*/
	/*They have to be initialized this way. A bit odd! */
	for (k = 0; k < 15; k++){
		inparm[k]=0.0;
		outparm[k]=0.0;
	}

	/* Finally, Call GCTP */
	inzone = utmzone;
	incoor[0] = ux;
	incoor[1] = uy;
	
	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
	     &ipr, efile, &jpr, pfile,
	     outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
	     fn27, fn83,&iflg);

	*lon=outcoor[0];
	*lat=outcoor[1];
}

/*******************************************************************************
NAME:     lonlat2utm()
*******************************************************************************/
void lonlat2utm(int utmzone, double lon, double lat, double *ux, double *uy)
{

	/* INPUT PROJ PARAMS */
	double incoor[2];       //input lon/lat
	long insys = 0;         //geographic
	long inzone = -1;       //Ignored for 
	double inparm[15];      //parameters 
	long inunit = 4;        //degrees of arc
	long indatum = 12;      //WGS 84

	/* OUTPUT PROJ PARAMS */
	double outcoor[2];      //output UTM x, y
	long outsys = 1;        //UTM
	long outzone;           //UTM ZONE number. 
	double outparm[15];     //parameters for UTM
	long outunit = 2;       //meters
	long outdatum = 12;     //WGS 84

	int k;

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */

	/*Projection parameter redirection*/
	long jpr = -1;                       /*don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   /*file to print the returned projection parameters*/

	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */


	/*Initialising parameters of both projections all to zeros first. No further initialized for UTM.*/
	/*They have to be initialized this way. A bit odd! */
	for (k = 0; k < 15; k++){
		inparm[k]=0.0;
		outparm[k]=0.0;
	}

	/* Finally, Call GCTP */
	outzone = utmzone;
	incoor[0] = lon;
	incoor[1] = lat;
	
	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
	     &ipr, efile, &jpr, pfile,
	     outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
	     fn27, fn83,&iflg);

	*ux=outcoor[0];
	*uy=outcoor[1];
}


void utm2utm(int inutmzone, double inx, double iny, int oututmzone, double *outx, double *outy)
{
	/* INTPUT PROJ PARAMS */
	double incoor[2];      //input UTM x, y
	long insys = 1;        //UTM
	long inzone;           //UTM ZONE number. 
	double inparm[15];     //parameters for UTM
	long inunit = 2;       //meters
	long indatum = 12;     //WGS 84

	/* OUTPUT PROJ PARAMS */
	double outcoor[2];     //output coordinates to be returned
	long outsys = 1;       
	long outzone ;        
	double outparm[15];
	long outunit = 2;     
	long outdatum = 12;   

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */

	/*Projection parameter redirection*/
	long jpr = -1;                       	/*don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   	/*file to print the returned projection parameters*/

	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file containing  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file containing  nad27 parameters.  requered by gctp  */


	/*Initialising parameters of both projections all to zeros first. No further initialized for UTM.*/
	/*They have to be initialized this way. A bit odd! */
	int k;
	for (k = 0; k < 15; k++){
		inparm[k]=0.0;
		outparm[k]=0.0;
	}

	/* Finally, Call GCTP */
	inzone = inutmzone;
	outzone = oututmzone;
	incoor[0]= inx;
	incoor[1]= iny;

	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
	     &ipr, efile, &jpr, pfile,
	     outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
	     fn27, fn83,&iflg);

	*outx=outcoor[0];
	*outy=outcoor[1];
}


/* Sinusoidal to geographic via direct inversion */
int sin2geo(double sx, double sy, double *lon, double *lat)
{	double Re = 6371007.181;
	double pi = atan(1)*4;
	//double pi = 3.1415926535897932384;
	double eps = pow(10, -16);

	/* In radiance */
	*lat = sy / Re;
	if (*lat > pi/2)	/* Out of map */
		return -1;
	else if (fabs(sx) >= Re * pi * cos(*lat))	/* Out of map */
		return -1;

	if (fabs(*lat) > pi/2 - eps) 
		*lon = 0;
	else 
		*lon = sx / Re / cos(*lat);

	/* In degrees */
	(*lat) *= (180/pi);
	(*lon) *= (180/pi);
	return 0;
}

/* Sinusoidal to geographic via gctp */
void sin2geo_gctp(double sx, double sy, double *lon, double *lat)
{
	/* Input params */
	double incoor[2];     /* input coordinates to be passed in */
	long insys = 16;      /* sinusoidal Equal Area */
	long inzone = -1;     /* Not required */
	double inparm[15];
	long inunit = 2;      /* meters */
	long indatum = 12;    /* WGS 84 */

    	/* Output params */
	double outcoor[2];    	 /* output lon/lat */
	long outsys = 0;         /* geographic */
	long outzone = -1;       /* Ignored */
	double outparm[15];      
	long outunit = 4;        /* degrees of arc */
	long outdatum = 12;      /* WGS 84 */

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */
	
	/*Projection parameter redirection*/
	long jpr = -1;                         /* don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   /* file to print the returned projection parameters to*/
	
	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
    
	/* initialising parameters of both projections all to zeros */ 
	int k;
	for (k = 0; k < 15; k++){
	       inparm[k]=0.0;
	       outparm[k]=0.0;
	}
    
	/* Parameters for sinusoidal*/
	inparm[0]=6371007.181;    	/* major elipsoid axis a (standard for the datum) */
	inparm[1]=0.0;        		/* minor elipsoid axis b (standard for the datum) */
	
	incoor[0]= sx;
	incoor[1]= sy;

    
	/* Call GCTP -----------------*/
	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
		&ipr, efile, &jpr, pfile,
		outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
		fn27, fn83,&iflg);
    
	*lon=outcoor[0];
	*lat=outcoor[1];
}


/* Sinusoidal to UTM */
void sin2utm(double sx, double sy, int utmzone, double *ux, double *uy)
{
	/* Input params */
	double incoor[2];     /* input coordinates to be passed in */
	long insys = 16;      /* sinusoidal Equal Area */
	long inzone = -1;     /* Not required */
	double inparm[15];
	long inunit = 2;      /* meters */
	long indatum = 12;    /* WGS 84 */

    	/* Output params */
	double outcoor[2];    	 /* output utm */
	long outsys = 1;         /* utm */
	long outzone;       	
	double outparm[15];      
	long outunit = 2;        /* meters */
	long outdatum = 12;      /* WGS 84 */

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */
	
	/*Projection parameter redirection*/
	long jpr = -1;                         /* don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   /* file to print the returned projection parameters to*/
	
	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
    
	/* initialising parameters of both projections all to zeros */ 
	int k;
	for (k = 0; k < 15; k++){
	       inparm[k]=0.0;
	       outparm[k]=0.0;
	}
    
	/* Parameters for sinusoidal*/
	inparm[0]=6371007.181;    	/* major elipsoid axis a (spherical for sinusoidal in fact) */
	inparm[1]=0.0;        		/* minor elipsoid axis b (standard for the datum) */
	inparm[2] = 0;			/* standard paralel */
	inparm[4] = 0;			/* central meridian */
	
	incoor[0]= sx;
	incoor[1]= sy;
	outzone = utmzone;

    
	/* Call GCTP -----------------*/
	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
		&ipr, efile, &jpr, pfile,
		outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
		fn27, fn83,&iflg);
    
	*ux=outcoor[0];
	*uy=outcoor[1];
}

/* UTM to sinusoidal */
void utm2sin(int utmzone, double ux, double uy, double *sx, double *sy) 
{
	/* Input params */
	double incoor[2];      	/* input coordinates to be passed in */
	long insys = 1;      	/* UTM */
	long inzone = -1;     	/* Not required */
	double inparm[15];
	long inunit = 2;      /* meters */
	long indatum = 12;    /* WGS 84 */

    	/* Output params */
	double outcoor[2];    	 /* output utm */
	long outsys = 16;        /* Sinusoidal */
	long outzone;       	
	double outparm[15];      
	long outunit = 2;        /* meters */
	long outdatum = 12;      /* WGS 84 */

	/*Error redirection*/
	long iflg;                            /* error number*/
	long ipr = 2;                         /* error message print to both terminal and file */
	char efile[] = "gctp_error_msg.txt";  /* name of error file   (required by gctp)   */
	
	/*Projection parameter redirection*/
	long jpr = -1;                         /* don't print the projection params*/
	char pfile[] = "gctp_proj_para.txt";   /* file to print the returned projection parameters to*/
	
	/* other GCTP parameters. Not used here */
	char fn27[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
	char fn83[] = " ";    /* file contaiming  nad27 parameters.  requered by gctp  */
    
	/* initialising parameters of both projections all to zeros */ 
	int k;
	for (k = 0; k < 15; k++){
	       inparm[k]=0.0;
	       outparm[k]=0.0;
	}
    
	/* Parameters for sinusoidal*/
	outparm[0]=6371007.181;    	/* major elipsoid axis a (spherical for sinusoidal in fact) */
	outparm[1]=0.0;        		/* minor elipsoid axis b (standard for the datum) */
	outparm[2] = 0;			/* standard paralel */
	outparm[4] = 0;			/* central meridian */
	
	inzone = utmzone;
	incoor[0]= ux;
	incoor[1]= uy;

    
	/* Call GCTP -----------------*/
	gctp(incoor, &insys, &inzone, inparm, &inunit, &indatum,
		&ipr, efile, &jpr, pfile,
		outcoor, &outsys, &outzone, outparm, &outunit, &outdatum,
		fn27, fn83,&iflg);
    
	*sx=outcoor[0];
	*sy=outcoor[1];
}
