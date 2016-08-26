
#ifndef THREAD_SAFE_HIGHGUI_H
#define THREAD_SAFE_HIGHGUI_H


#include <afxmt.h>		// Windows multithreading stuff
#include <ipl.h>		// must be included before including highgui.h
#include <highgui.h>
#undef resize			// to avoid highgui function conflicting with STL's vector method


// **NOTE** Highgui functions are NOT thread-safe. 
// Therefore, if using HighGui in a multi-threaded program, one must protect
// all calls to the HighGui functions with the critical section defined below.
// Usage is:
//		safe_highgui.Lock();
//		<call highgui function here>
//		safe_highgui.Unlock();



extern CCriticalSection safe_highgui;



#endif