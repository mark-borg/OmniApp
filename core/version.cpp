
#include "version.h"
#include <string.h>
#include <ipl.h>
#include <cv.h>
#include <ijl.h>



#define __VERSION__	"Version 2.5.00"
#pragma message( "**NOTE** Remember to check the version " __VERSION__ " is correct in " __FILE__ " !!!" )




Version::Version()
{
	strcpy( build, "Build: " __DATE__ " " __TIME__ );
	strcpy( version, __VERSION__ );

	// check libraries being used...
	const char *opencv_ver = 0, *dll_name = 0;
//	cvGetLibraryInfo( &opencv_ver, 0, &dll_name );			// No longer provided by OpenCV library
	opencv_ver = "?";
	dll_name = "?";

	const IPLLibVersion* ipl_ver = iplGetLibVersion();

	const IJLibVersion* ijl_ver = ijlGetLibVersion();

	strcpy( libraries, "Libraries loaded: \nOpenCV " );
	strcat( libraries, opencv_ver );
	strcat( libraries, " (" );
	strcat( libraries, dll_name );
	strcat( libraries, ")\nIPL " );
	strcat( libraries, ipl_ver->InternalVersion );
	strcat( libraries, " (" );
	strcat( libraries, ipl_ver->Name );
	strcat( libraries, ")\nIJL " );
	strcat( libraries, ijl_ver->Version );
	strcat( libraries, " " );
	strcat( libraries, ijl_ver->BuildDate );
	strcat( libraries, " (" );
	strcat( libraries, ijl_ver->Name );
	strcat( libraries, ")" );
}


