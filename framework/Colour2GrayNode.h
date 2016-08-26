
#ifndef COLOUR_2_GRAY_NODE_H
#define COLOUR_2_GRAY_NODE_H


#include "ProcessorNode.h"
#include "IplData.h"
#include "IplImageResource.h"
#include "..\tools\CvUtils.h"




template< class ALLOCATOR >
class Colour2GrayNode : public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;
	IplData output;


public:


	Colour2GrayNode( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;
	}



	~Colour2GrayNode()
	{
		if ( output.img != 0 )
			allocator->release( output.img );
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

		// determine type of gray image 
		IplImageResource img_typ = IplImageResource::getType( buffer->img );
		img_typ.channels = 1;				// set to gray

		// acquire output buffer
		output.frame = buffer->frame;
		output.img = allocator->acquire( img_typ );
		assert( output.img );


		// convert
#ifdef TRUE_GRAYSCALE

		// this uses a conversion: gr = r*0.212671 + g*0.715160 + b*0.072169
		iplColorToGray( buffer->img, output.img );

#else

		// this uses a conversion: gr = ( r + g*2 + b ) / 4
		assert( buffer->img->channelSeq[ 0 ] == 'B' &&
				buffer->img->channelSeq[ 1 ] == 'G' &&
				buffer->img->channelSeq[ 2 ] == 'R' && 
				buffer->img->channelSeq[ 3 ] == '\x00' );


		for ( int y = 0; y < buffer->img->height; y++ )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, buffer->img, y );
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_dst, output.img, y );

			for ( register int x = 0, x3 = 0; x < buffer->img->width; ++x, x3+=3 )
			{
				register unsigned char* bgr = &p_src[ x3 ];
				p_dst[ x ] = (unsigned char)( (bgr[2] >> 2) + (bgr[1] >> 1) + (bgr[0] >> 2) );
			}
		}

#endif

		// move result to sink buffer
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = output;
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( output.img != 0 )
		{
			allocator->release( output.img );
			output.img = 0;
		}
	}

};



#endif
