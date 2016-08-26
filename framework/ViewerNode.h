
#ifndef VIEWER_NODE_H
#define VIEWER_NODE_H


#include "ProcessorNode.h"
#include "IplData.h"


#ifndef LOG
#define LOG(A)
#endif




template< class ALLOCATOR, class GUI_OBJECT >
class ViewerNode : public ProcessorNode< IplData, IplData >
{
public:

	GUI_OBJECT* gui_obj;

	ALLOCATOR* allocator;



	ViewerNode( string my_name, ALLOCATOR* allocator, GUI_OBJECT* gui_object )
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
		IplData* buffer = & source_buffers[ 0 ];
		assert( buffer );

		assert( buffer->img );

		// pass on to GUI object...
		if ( gui_obj->m_hWnd && gui_obj->IsWindowVisible() )
		{
			gui_obj->update( *buffer );
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
