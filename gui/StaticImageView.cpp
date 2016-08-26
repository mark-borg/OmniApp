
#include "StaticImageView.h"
#include "..\tools\pview\pview.h"




CStaticImageView::CStaticImageView( CString title )
{
	do_paint = CreateEvent(	NULL, 
							false,		// automatic reset
							false,		// initially NOT signalled
							NULL );
	
	// used by pview tools when displaying thread information
	CString thread_name = "ICN: " + title;
	registerThreadName( this->threadID(), thread_name.operator LPCTSTR() );

	resume();
}

