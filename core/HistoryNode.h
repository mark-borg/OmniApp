
#ifndef HISTORY_NODE_H
#define HISTORY_NODE_H


#pragma warning( disable : 4786 )
 
#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "ObjectsInformation.h"



bool prepareOutputDir();
bool process( IplImage* img, ObjectList* objects, long frame );




template< class ALLOCATOR >
class HistoryNode
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;

	bool file_error;
	bool save_history;


public:



	HistoryNode( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		save_history = CfgParameters::instance()->doSaveHistory();
		file_error = false;

		if ( save_history )
		{
			file_error = ! prepareOutputDir();
			if ( file_error )
				save_history = false;
		}
	}



	~HistoryNode()
	{
	}
	


	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		IplImage* img = source_buffers[ 0 ].img;
		assert( img );
		assert( img->nChannels == 3 );
		assert( img->depth == IPL_DEPTH_8U );


		if ( save_history )
			process( img, objects, source_buffers[ 0 ].frame );


		// move input (unchanged) to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = source_buffers[ 0 ];
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		// terminate this node if file error...
		if ( file_error )
			terminate( RC_ERROR );
	}

};



#endif
