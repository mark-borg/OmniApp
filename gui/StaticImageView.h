
#ifndef STATIC_IMAGE_VIEW_H
#define STATIC_IMAGE_VIEW_H
#pragma once


#include "stdafx.h"
#include "Ipl2DecimatedBmp.h"
#include "..\framework\RunnableObject.h"
#include "..\framework\IplData.h"




class CStaticImageView : public CStatic, public RunnableObject
{
private:

	Ipl2DecimatedBmp converter;
	HANDLE do_paint;


public:

	CStaticImageView( CString title = "" );



	~CStaticImageView()
	{
		CloseHandle( do_paint );
	}



	virtual void update( const IplData image_data )
	{
		RECT r;
		GetClientRect( &r );
		converter.setOutputSize( r );

		converter.update( image_data.img );

		if ( WaitForSingleObject( do_paint, 0 ) != WAIT_OBJECT_0 )
			SetEvent( do_paint );
	}



	UINT loop()
	{
		while( true )
		{
			if ( WaitForSingleObject( do_paint, A_LONG_TIME * 2 ) == WAIT_OBJECT_0 )
				converter.paint( this );

			checkIfMustStop( RC_ABORTED );		// told to stop?

			if ( m_hWnd == 0 )					// has window been deleted?
				terminate( RC_ABORTED );

		}
	}



	void reset()
	{
		converter.reset();
	}

};



#endif
