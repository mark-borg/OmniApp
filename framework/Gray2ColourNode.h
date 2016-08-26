
#ifndef GRAY_2_COLOUR_NODE_H
#define GRAY_2_COLOUR_NODE_H


#include "ProcessorNode.h"
#include "IplData.h"
#include "IplImageResource.h"



#ifndef LOG
#define LOG(A)
#endif



// good values for the colour factors are 1.0,1.0,1.0

template< class ALLOCATOR >
class Gray2ColourNode : public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;
	IplData output;

	float RED_FACTOR;
	float GREEN_FACTOR;
	float BLUE_FACTOR;


public:


	Gray2ColourNode( string my_name, ALLOCATOR* allocator, float red_factor, float green_factor, float blue_factor )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;
		RED_FACTOR = red_factor;
		GREEN_FACTOR = green_factor;
		BLUE_FACTOR = blue_factor;
	}



	~Gray2ColourNode()
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

		// determine type of image 
		IplImageResource img_typ = IplImageResource::getType( buffer->img );
		img_typ.channels = 3;				// set to colour

		// acquire output buffer
		output.frame = buffer->frame;
		output.img = allocator->acquire( img_typ );
		assert( output.img );

		// convert
		iplGrayToColor( buffer->img, output.img, RED_FACTOR, GREEN_FACTOR, BLUE_FACTOR );

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
