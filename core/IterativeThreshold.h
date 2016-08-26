
#ifndef ITERATIVE_THRESHOLD_H
#define ITERATIVE_THRESHOLD_H


#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"




bool fnIterativeThreshold( IplImage* src, IplImage* dst, 
						   int* threshold,
						   int initial_threshold = 128 );



template< class ALLOCATOR >
class IterativeThreshold
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;


	IplData wrk1, wrk2;



public:


	IterativeThreshold( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

	}



	~IterativeThreshold()
	{
		if ( wrk1.img != 0 )
			allocator->release( wrk1.img );
		if ( wrk2.img != 0 )
			allocator->release( wrk2.img );
	}



	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		// get source
		IplData* buffer = & source_buffers[ 0 ];
		assert( buffer );
		assert( buffer->img );

		
		// acquire result image buffers
		IplImageResource img_typ;
		img_typ = IplImageResource( buffer->img->width, buffer->img->height, IPL_DEPTH_8U, 1 );

		assert( wrk1.img == 0 );
		wrk1.img = allocator->acquire( img_typ );
		assert( wrk1.img );

		assert( wrk2.img == 0 );
		wrk2.img = allocator->acquire( img_typ );
		assert( wrk2.img );


		// some extra validation on the incoming image
		assert( buffer->img->nChannels == 1 );
		assert( buffer->img->depth == IPL_DEPTH_8U );


		// initial guess for background are corner pixels of image
		unsigned char v1, v2, v3, v4;
		iplGetPixel( buffer->img, 0, 0, &v1 );
		iplGetPixel( buffer->img, buffer->img->width-1, 0, &v2 );
		iplGetPixel( buffer->img, 0, buffer->img->height-1, &v3 );
		iplGetPixel( buffer->img, buffer->img->width-1, buffer->img->height-1, &v4 );
		
		int T0 = MAX( MAX( MAX( v1, v2 ), v3 ), v4 ) + 1;


		// process
		wrk1.frame = buffer->frame;
		wrk2.frame = buffer->frame;
		int T = -1;
		bool ok = fnIterativeThreshold( buffer->img, wrk1.img, &T, T0 );
		iplClose( wrk1.img, wrk2.img, 2 );
		iplOpen( wrk2.img, wrk1.img, 2 );


		// move result to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = wrk1;
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( wrk1.img != 0 )
		{
			allocator->release( wrk1.img );
			wrk1.img = 0;
		}
		if ( wrk2.img != 0 )
		{
			allocator->release( wrk2.img );
			wrk2.img = 0;
		}
	}

};



#endif
