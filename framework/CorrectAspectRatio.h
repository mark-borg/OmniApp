
#ifndef CORRECT_ASPECT_RATIO_H
#define CORRECT_ASPECT_RATIO_H


#include "ProcessorNode.h"
#include "IplData.h"
#include "IplImageResource.h"
#include "..\tools\CvUtils.h"





template< class ALLOCATOR >
class CorrectAspectRatio : public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;

	IplImageResource map_typ;
	IplImage* m_geom_map_x;
	IplImage* m_geom_map_y;
	
	IplData output;

	float aspect_ratio;
	int src_width, src_height;
	int dst_width, dst_height;

	int interpolation;


public:


	CorrectAspectRatio( string my_name, 
						ALLOCATOR* allocator,
						int src_width, int src_height,
						int dst_width, int dst_height,
						int interpolation = IPL_INTER_LINEAR )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		assert( src_width > 0 && src_height > 0 && dst_width > 0 && dst_height > 0 );
		this->src_width		= src_width;
		this->src_height	= src_height;
		this->dst_width		= dst_width;
		this->dst_height	= dst_height;
		aspect_ratio		= dst_width / (float) dst_height;

		this->interpolation	= interpolation;

		m_geom_map_x = 0;
		m_geom_map_y = 0;
	}



	~CorrectAspectRatio()
	{
		if ( m_geom_map_x != 0 )
			allocator->release( m_geom_map_x );
		if ( m_geom_map_y != 0 )
			allocator->release( m_geom_map_y );
		if ( output.img != 0 )
			allocator->release( output.img );
	}



	void initMapping( int in_width, int in_height )
	{
#ifdef USE_IPL_REMAP_WITH_LUT

		int dst_height	= src_height,
			dst_width	= lrintf( src_height * aspect_ratio );


		int out_height	= in_height,
			out_width	= in_width * dst_width / src_width;

		// create geometric maps
		map_typ = IplImageResource( out_width, out_height, IPL_DEPTH_32F, 1 );

		if ( m_geom_map_x != 0 )
			allocator->release( m_geom_map_x );
		if ( m_geom_map_y != 0 )
			allocator->release( m_geom_map_y );
		
		m_geom_map_x = allocator->acquire( map_typ );
		assert( m_geom_map_x );
		m_geom_map_y = allocator->acquire( map_typ );
		assert( m_geom_map_y );


		COMPUTE_IMAGE_ROW_PTR( float, map_x, m_geom_map_x, 0 );
		COMPUTE_IMAGE_ROW_PTR( float, map_y, m_geom_map_y, 0 );

		for ( int x = 0; x < out_width; ++x )
		{
			map_x[ x ] = x / (float) out_width * (float) in_width;
			map_y[ x ] = 0;
		}

		for ( int y = 1; y < out_height; ++y )
		{
			COMPUTE_IMAGE_ROW_PTR( float, first_x, m_geom_map_x, 0 );
			COMPUTE_IMAGE_ROW_PTR( float, map_x, m_geom_map_x, y );
			COMPUTE_IMAGE_ROW_PTR( float, map_y, m_geom_map_y, y );

			for ( int x = 0; x < out_width; ++x )
			{
				map_x[ x ] = first_x[ x ];
				map_y[ x ] = y;
			}
		}

#endif
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

		
#ifdef USE_IPL_REMAP_WITH_LUT
		if ( m_geom_map_x == 0 )
			initMapping( buffer->img->width, buffer->img->height );


		// determine type of image 
		IplImageResource img_typ = IplImageResource::getType( buffer->img );
		img_typ.width = m_geom_map_x->width;

		// acquire output buffer
		assert( output.img == 0 );
		output.img = allocator->acquire( img_typ );
		assert( output.img );


		assert( m_geom_map_x );
		assert( m_geom_map_y );

		// resize
		output.frame = buffer->frame;

		iplRemap( buffer->img, m_geom_map_x, m_geom_map_y, output.img, 
				  interpolation );
#else
		// determine type of image 
		IplImageResource img_typ = IplImageResource::getType( buffer->img );
		img_typ.width = buffer->img->width;
		img_typ.height = ( buffer->img->height * dst_height ) / src_height;

		// acquire output buffer
		assert( output.img == 0 );
		output.img = allocator->acquire( img_typ );
		assert( output.img );

		// resize
		output.frame = buffer->frame;

		iplResize(	buffer->img, output.img, 
					dst_width, src_width, 
					dst_height, src_height, 
					interpolation );
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
