
#ifndef EDGE_DETECTOR_H
#define EDGE_DETECTOR_H


#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"




void SimplifyExternalContours( IplImage* img );




template< class ALLOCATOR >
class EdgeDetector
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;


	IplData wrk1;



public:


	EdgeDetector( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;
	}



	~EdgeDetector()
	{
		if ( wrk1.img != 0 )
			allocator->release( wrk1.img );
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


		// some extra validation on the incoming image
		assert( buffer->img->nChannels == 1 );
		assert( buffer->img->depth == IPL_DEPTH_8U );


		// process
		wrk1.frame = buffer->frame;
		cvCanny( buffer->img, wrk1.img, 0, 255, 5 );
		SimplifyExternalContours( wrk1.img );


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
	}

};



#endif
