
#ifndef SLIDESHOW_VIEWER_NODE_H
#define SLIDESHOW_VIEWER_NODE_H


#include "ProcessorNode.h"
#include "IplData.h"


#ifndef LOG
#define LOG(A)
#endif




template< class ALLOCATOR, class GUI_OBJECT, int DELAY >
class SlideshowViewerNode : public ProcessorNode< IplData, IplData >
{
public:

	GUI_OBJECT* gui_obj;

	ALLOCATOR* allocator;



	SlideshowViewerNode( string my_name, ALLOCATOR* allocator, GUI_OBJECT* gui_object )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		gui_obj = gui_object;
		assert( gui_obj );
		
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
		if ( gui_obj->m_hWnd && gui_obj->IsWindowVisible() )
		{
			for ( int k = 0; k < source_buffers.size(); ++k )
			{
				IplData* buffer = & source_buffers[ k ];
				assert( buffer );

				assert( buffer->img );

				// pass on to GUI object...
				gui_obj->update( *buffer );
				Sleep( DELAY );
			}
			Show();
		}
		else
			Hide();
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );
	}
};



#endif
