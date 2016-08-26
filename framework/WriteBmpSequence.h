
#ifndef WRITE_BMP_SEQUENCE_H
#define WRITE_BMP_SEQUENCE_H


#include "ProcessorNode.h"
#include "..\tools\ThreadSafeHighgui.h"
#include "IplData.h"


#ifndef LOG
#define LOG(A)
#endif


// LIMITATIONS OF THIS CLASS:
// --------------------------
// This processor node class assumes that one source node is connected to it.
// It will save the image data as a BMP file using OpenCV's HighGUI library.
// If more than one source node is connected, then only the image data
// received from the first one, is saved to disk. The data received from the
// rest of the source nodes, is ignored.
// The supplied path_format can contain a file path and name and it can contain
// an int format string (e.g. %d) that will be exchanged for the current frame
// number for that image.
// The HighGui library is not thread-safe, so a critical section is used to
// protect against concurrent calls to the HighGui library (defined in 
// ThreadSafeHighgui.h).
// If more than one instance of this node is included in a pipeline, some delay 
// may occur because of this. 
// In addition, if any other processor node is used that calls HighGui 
// functions, it must use the same critical section as used in this file, or
// else a run-time error may occur.




template< class ALLOCATOR >
class WriteBmpSequence : public ProcessorNode< IplData, IplData >
{
protected:

	char filename[ MAX_PATH ];
	char path_format[ MAX_PATH ];

	ALLOCATOR* allocator;


public:


	WriteBmpSequence( string name, string path_format, 
					  ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( name )
	{
		assert( path_format.length() <= MAX_PATH );
		strcpy( this->path_format, path_format.c_str() );

		assert( allocator );
		this->allocator = allocator;
	}


	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}


	virtual void processData()
	{
		IplData* buffer = & source_buffers[ 0 ];
		assert( buffer );

		// save to disk
		sprintf( filename, path_format, buffer->frame );

		safe_highgui.Lock();
		int rc = cvSaveImage( filename, buffer->img );
		safe_highgui.Unlock();
		assert( rc >= HG_OK );
	}



	virtual void finalise()
	{
		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )				// in case source node died
			allocator->release( source_buffers[k].img );
	}

};



#endif
