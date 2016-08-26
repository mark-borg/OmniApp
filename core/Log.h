
#ifndef LOG_H
#define LOG_H


#include "..\tools\LogHandler.h"
#include <strstream>
#include <assert.h>




class Log : public LogHandler
{
private:

	strstream buffer;

	void* theApp;


	void clear();



public:

	Log();


	virtual ostream& operator() ( );


	void dump( unsigned long colour = 0xffffffff );

};



// the Log instance(s)
extern Log LogOne;


#undef LOG
#undef CLOG

#define LOG( MSG )			{	\
								LogOne() << MSG;	\
								LogOne.dump();		\
							}

#define CLOG( MSG, CLR )	{	\
								LogOne() << MSG;	\
								LogOne.dump( CLR );	\
							}
						


#endif

