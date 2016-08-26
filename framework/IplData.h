
#ifndef IPL_DATA_H
#define IPL_DATA_H


#include <ipl.h>



struct IplData
{
	IplImage* img;
	int frame;
	bool ignore;

	IplData()
	{ 
		clear();
	}


	IplData( IplImage* m, int f, bool ignore_the_data = false )
		: img( m ), frame( f ), ignore( ignore_the_data )
	{
	}


	void clear()
	{
		img = 0;
		frame = -1;
		ignore = false;
	}

};



#endif

