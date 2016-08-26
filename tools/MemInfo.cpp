

#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <iostream>
#include <iomanip>
#include "MemInfo.h"
#include "..\core\log.h"

using namespace std;




void showMemInfo()
{
	// disk and memory stats
	double freeb = -1, freem = -1, freep = -1;

	ULARGE_INTEGER free_bytes, total_bytes, free_disk_bytes;
	if ( GetDiskFreeSpaceEx( "\\", &free_bytes, &total_bytes, &free_disk_bytes ) )
		freeb = __int64( free_bytes.QuadPart ) / 1073741824.0;

	MEMORYSTATUS mem_stat;
	GlobalMemoryStatus( &mem_stat );

	freem = mem_stat.dwAvailPhys / 1048576.0;
	freep = mem_stat.dwAvailPageFile / 1048576.0;

	LOG( "" );
	LOG( "---- Disk & Memory Space ------------------" );
	LOG( "Disk free : " << setprecision(2) << setiosflags( ios::fixed ) << freeb << " Gb\n"
		 << "Memory free : " << setprecision(2) << setiosflags( ios::fixed ) << freem << " Mb  (" 
		 << setprecision(6) << mem_stat.dwAvailPhys << "b)\n"
		 << "Pagefile free: " << setprecision(2) << setiosflags( ios::fixed ) << freep << " Mb  (" 
		 << setprecision(6) << mem_stat.dwAvailPageFile << "b)" );

}


