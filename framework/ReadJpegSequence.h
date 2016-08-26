
#ifndef READ_JPEG_SEQUENCE_H
#define READ_JPEG_SEQUENCE_H


#include "ProcessorNode.h"
#include "IplData.h"
#include "IplImageResource.h"
#include "..\tools\ijl_ipl.h"
#include "..\tools\timer.h"




// LIMITATIONS OF THIS CLASS:
// --------------------------
// Assumes the sequence of JPEGS to be read, have same image properties
// throughout the whole sequence (size, depth, number of channels). This 
// assumption is used to do some initialisation on the first image read 
// and then to cache this for the rest of the images. This assumption was
// made for speed reasons.


template< class ALLOCATOR >
class ReadJpegSequence : public ProcessorNode< IplData, IplData >
{
protected:

	char path[ MAX_PATH ];
	char path_format[ MAX_PATH ];
	
	long seq_num;
	long last_num;
	bool forever;
	int	 read_step;

	CvRect roi;
	bool use_roi;

	IplData buffer;	

	ALLOCATOR* allocator;

	IplImageResource typ;
	bool first_time;


public:


	ReadJpegSequence(	string name, string source_path_format, 
						ALLOCATOR* allocator,
						long start_num = 0, 
						long stop_num = -1,
						int  read_step = 1,
						CvRect* roi = 0 )
		:	ProcessorNode< IplData, IplData >( name )
	{
		assert( start_num >= 0 );
		assert( stop_num == -1 || stop_num >= 0 && stop_num >= start_num );
		assert( read_step > 0 );
		
		seq_num = start_num;
		last_num = stop_num;
		forever = stop_num == -1;
		this->read_step = read_step;

		assert( source_path_format.length() <= MAX_PATH );
		strcpy( path_format, source_path_format.c_str() );

		assert( allocator );
		this->allocator = allocator;

		if ( use_roi = ( roi != 0 ) )
			this->roi = *roi;

		first_time = true;
	}



	virtual void finalise()
	{
		seq_num += read_step;

		assert( buffer.img );
		allocator->release( buffer.img );

		if ( ! forever )
			if ( seq_num > last_num )
				terminate( RC_SUCCESS );
	}


	
	virtual void waitForData()
	{
		sprintf( path, path_format, seq_num );


		if ( first_time )
		{
			IplImage* hdr = use_roi ? ReadHeaderJpegROI( path, roi )
									: ReadHeaderJpeg( path );
			if ( ! hdr )
			{
				buffer.img = 0;
				seq_num = 0;
				terminate( RC_ERROR );
				return;
			}

			assert( hdr );
			typ = IplImageResource::getType( hdr );

			iplDeallocate( hdr, IPL_IMAGE_HEADER );

			first_time = false;
		}

		buffer.frame = seq_num;
		buffer.img = allocator->acquire( typ );
		assert( buffer.img );

		IplImage* ptr = use_roi ? ReadJpegROI( path, roi, false, buffer.img )
								: ReadJpeg( path, false, buffer.img );

		if ( ptr == 0 )						// terminate if no more JPEG files
			terminate( RC_SUCCESS );

		assert( ptr );
		assert( ptr == buffer.img );
	}



	virtual void processData()
	{
		// no processing - just straight copy through
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = buffer;
	}



	long getFrameNumber()
	{
		return seq_num;
	}

};



#endif
