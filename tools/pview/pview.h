
#ifndef PVIEW_H
#define PVIEW_H

#include <ostream>
#include <map>



void showProcessThreadInfo( std::ostream& strm, const char* app_name );



// since Windows does not keep a name for threads (not like processes), an 
// optional functionality is provided through this function to save/store a
// thread name for threads. The thread name is associated with the thread ID
// (which Windows keeps track of). When getting performance information about 
// threads, the code in this folder will do an additional lookup into this
// structure and will display the thread name (if any) in addition to the 
// thread ID. 
//
// **NOTE**
// Please note that it is the user responsibility to keep track of which 
// threads to register a name for. And also, when a thread dies, it is 
// recommended that the registration is removed (by re-registering the thread's
// ID with an empty string), since Windows can re-cycle thread IDs for the 
// same process.
void registerThreadName( DWORD thread_id, std::string thread_name );




#endif

