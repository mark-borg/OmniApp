
#ifndef IPL_IMAGE_RESOURCE_H
#define IPL_IMAGE_RESOURCE_H


#include <ipl.h>
#include <cv.h>
#include <iostream>

using namespace std;




class IplImageResource
{
public:

	typedef IplImage*	ptr_type;
		

	int width, height;
	int depth;
	int channels;



	IplImageResource( int w, int h, int d, int c )
	{
		width = w;
		height = h;
		depth = d;
		channels = c;
	}



	IplImageResource()
	{
		width = height = channels = depth = -1;
	}



	int compare( const IplImageResource& b ) const
	{
#define IMAGE_RES_COMPARE( FIELD )					\
		if ( this->FIELD < b.FIELD )	return -1;	\
		if ( this->FIELD > b.FIELD )	return 1;	

		
		IMAGE_RES_COMPARE( width );
		IMAGE_RES_COMPARE( height );
		IMAGE_RES_COMPARE( depth );
		IMAGE_RES_COMPARE( channels );


#undef IMAGE_RES_COMPARE

		return 0;
	}



	IplImage* allocate()
	{
		return cvCreateImage( cvSize( width, height ), depth, channels );
	}



	IplImage* clear( IplImage* ptr )
	{
		assert( ptr );
		
		if ( depth == IPL_DEPTH_32F )
			iplSetFP( ptr, 0 );
		else
			iplSet( ptr, 0 );

		return ptr;
	}



	void deallocate( IplImage* ptr )
	{
		assert( ptr );
		cvReleaseImage( &ptr );
	}



	long memoryUsed();



	static IplImageResource getType( IplImage* ptr )
	{
		assert( ptr );
		return IplImageResource( ptr->width, ptr->height, ptr->depth, ptr->nChannels );
	}



	friend ostream& operator<< ( ostream&, const IplImageResource& );
	friend bool operator== ( const IplImageResource&, const IplImageResource& );
};




ostream& operator<< ( ostream& strm, const IplImageResource& res );


bool operator== ( const IplImageResource& r1, const IplImageResource& r2 );



#endif

