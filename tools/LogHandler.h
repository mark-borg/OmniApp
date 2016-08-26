
#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H


#include <ostream>

using namespace std;




class LogHandler
{
public:

	
	virtual ostream& operator() ( ) = 0;


};



#endif

