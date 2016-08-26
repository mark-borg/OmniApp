
#include "stdafx.h"
#include <ipl.h>
#include <cv.h>
#include "FloatingOmniImageView.h"
#include "..\core\VirtualCameraController.h"



BEGIN_MESSAGE_MAP( CFloatingOmniImageView, CFloatingImageView )
	//{{AFX_MSG_MAP(CFloatingOmniImageView)
	ON_COMMAND(ID_TOGGLE_BORDERS, OnToggleBorders)
	ON_COMMAND(ID_PERSPECTIVE_1, OnPerspective1)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_PERSPECTIVE_2, OnPerspective2)
	ON_COMMAND(ID_PERSPECTIVE_3, OnPerspective3)
	ON_COMMAND(ID_PERSPECTIVE_4, OnPerspective4)
	ON_COMMAND(ID_PERSPECTIVE_5, OnPerspective5)
	ON_COMMAND(ID_PERSPECTIVE_6, OnPerspective6)
	ON_COMMAND(ID_TRACK_OBJECT, OnTrackObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




void CFloatingOmniImageView::CreateToolbar()
{
	if (m_toolBar.Create(this))
	{
		const UINT cmds[] = {	ID_SAVE_PICTURE, 
								ID_SEPARATOR,
								ID_TOGGLE_BORDERS };
		m_toolBar.LoadBitmap( IDR_OMNI_FLOAT_WINDOW_TOOLBAR );
		m_toolBar.SetButtons( (const UINT*) &cmds, 3 );
	}
}




void CFloatingOmniImageView::drawBorders( const IplData image_data )
{
	if ( ! borders_enabled )
		return;

	if ( Pipeline::instance()->isInitialising() )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		CfgParameters* params = CfgParameters::instance();

		unsigned char RED[] = { 0, 0, 255 };

		
		IplImage* img = image_data.img;
		assert( img );
				

		int cx = params->getMirrorCentreX(),
			cy = params->getMirrorCentreY();
		cvLine( img,
				cvPoint( cx - 10, cy ),
				cvPoint( cx + 10, cy ),
				CV_RGB(0,0,255) );			//MM! 0xff0000 );
		cvLine( img,
				cvPoint( cx, cy - 10 ),
				cvPoint( cx, cy + 10 ),
				CV_RGB(0,0,255) );			//MM! 0xff0000 );

		cvCircle( img, cvPoint( cx, cy ), params->getMirrorBoundary(), CV_RGB(0,0,255) );		//MM! 0xff0000 );


		int k = 0;

		dewarper_type* dewarper = 0;

		do
		{
			dewarper = Pipeline::instance()->getPerspectiveNode( k );

			if ( dewarper )
			{
				// draw horizontal borders
				COMPUTE_IMAGE_ROW_PTR( float, map_row1_x, dewarper->getGeomMapX(), 0 );
				COMPUTE_IMAGE_ROW_PTR( float, map_row1_y, dewarper->getGeomMapY(), 0 );
				COMPUTE_IMAGE_ROW_PTR( float, map_row2_x, dewarper->getGeomMapX(), dewarper->height() -1 );
				COMPUTE_IMAGE_ROW_PTR( float, map_row2_y, dewarper->getGeomMapY(), dewarper->height() -1 );

				const int border_step = dewarper->getWidth() / 20;
				int vx = border_step;
				bool do_quit = false;

				CvScalar colour_code = dewarper->getColourCode();

				while ( vx < dewarper->getWidth() )
				{
					cvLine( img,
							cvPoint( lrintf( map_row1_x[ vx - border_step ] ), lrintf( map_row1_y[ vx - border_step ] ) ),
							cvPoint( lrintf( map_row1_x[ vx ] ), lrintf( map_row1_y[ vx ] ) ),
							colour_code );
					
					cvLine( img,
							cvPoint( lrintf( map_row2_x[ vx - border_step ] ), lrintf( map_row2_y[ vx - border_step ] ) ),
							cvPoint( lrintf( map_row2_x[ vx ] ), lrintf( map_row2_y[ vx ] ) ),
							colour_code );

					vx += border_step;

					if ( do_quit )
						break;

					if ( vx >= dewarper->getWidth() )
					{
						vx = dewarper->getWidth()-1;
						do_quit = true;
					}
				}


				// vertical borders
				cvLine( img, 
						cvPoint( lrintf( map_row1_x[ 0 ] ), lrintf( map_row1_y[ 0 ] ) ),
						cvPoint( lrintf( map_row2_x[ 0 ] ), lrintf( map_row2_y[ 0 ] ) ), 
						colour_code );
				cvLine( img, 
						cvPoint( lrintf( map_row1_x[ dewarper->width()-1 ] ), lrintf( map_row1_y[ dewarper->width()-1 ] ) ),
						cvPoint( lrintf( map_row2_x[ dewarper->width()-1 ] ), lrintf( map_row2_y[ dewarper->width()-1 ] ) ), 
						colour_code );
			}

			++k;
		}
		while( dewarper );
	}
	Pipeline::instance()->safe_state.Unlock();
}




void CFloatingOmniImageView::update( const IplData image_data )
{
	assert( image_data.img );

	if ( borders_enabled )
	{
		IplImageResource res = IplImageResource::getType( image_data.img );

		IplData tmp;
		tmp.frame = image_data.frame;
		tmp.img = Pipeline::instance()->MemPool.acquire( res );
		assert( tmp.img );

		iplCopy( image_data.img, tmp.img );

	
		drawBorders( tmp );


		// now let parent draw the image
		CFloatingImageView::update( tmp );


		Pipeline::instance()->MemPool.release( tmp.img );
	}
	else
	{
		CFloatingImageView::update( image_data );
	}
}




void CFloatingOmniImageView::OnToggleBorders() 
{
	borders_enabled = ! borders_enabled;
}




void CFloatingOmniImageView::customisePopupMenu( CMenu* pMenu )
{
	pMenu->LoadMenu( IDR_FLOAT_WINDOW_MENU_2 );

	int method = CfgParameters::instance()->getCameraControlMethod();

	if ( method != CfgParameters::MANUAL_CONTROL_METHOD_1 )
		pMenu->EnableMenuItem( ID_TRACK_OBJECT, MF_GRAYED );

}




void CFloatingOmniImageView::OnChangeViewPosition( int window_id )
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	Pipeline::PerspectiveDewarper_Type* dewarper = Pipeline::instance()->getPerspectiveNode( window_id );
	if ( dewarper && mouse_down_point.x != -1 )
	{ 
		int x = mouse_down_point.x - origin_x - scroll_x;
		int y = mouse_down_point.y - origin_y - scroll_y;

		float** p_azimuth_map = CfgParameters::instance()->getAzimuthMapPtrs();
		float** p_altitude_map = CfgParameters::instance()->getAltitudeMapPtrs();

		float vfov = dewarper->getVFOV();

		float new_az = p_azimuth_map[ y ][ x ] + 180;
		float new_el = p_altitude_map[ y ][ x ] - vfov /2;

		dewarper->adjustPan( new_az );
		dewarper->adjustTilt( new_el );
		dewarper->updateMapping();
	}
}




void CFloatingOmniImageView::OnRButtonDown( UINT nFlags, CPoint point ) 
{
	// save mouse location - this will be used later when changing the position
	// of a perspective camera view.
	mouse_down_point = point;
	
	CFloatingImageView ::OnRButtonDown( nFlags, point );
}




void CFloatingOmniImageView::OnPerspective1() 
{
	OnChangeViewPosition( 0 );
}




void CFloatingOmniImageView::OnPerspective2() 
{
	OnChangeViewPosition( 1 );
}




void CFloatingOmniImageView::OnPerspective3() 
{
	OnChangeViewPosition( 2 );
}




void CFloatingOmniImageView::OnPerspective4() 
{
	OnChangeViewPosition( 3 );
}




void CFloatingOmniImageView::OnPerspective5() 
{
	OnChangeViewPosition( 4 );
}




void CFloatingOmniImageView::OnPerspective6() 
{
	OnChangeViewPosition( 5 );
}




void CFloatingOmniImageView::OnTrackObject() 
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	if ( ::objects->size() == 0 )
		return;

	if ( mouse_down_point.x != -1 )
	{ 
		int x = mouse_down_point.x - origin_x - scroll_x;
		int y = mouse_down_point.y - origin_y - scroll_y;

		double best_distance = 100;
		int obj_id = -1;

		for ( int o = objects->first(); o != -1; o = objects->next( o ) )
		{
			Object* obj = objects->getAt( o );
			
			double d1 = obj->centre.x - x;
			double d2 = obj->centre.y - y;
			double d = sqrt( d1 * d1 + d2 * d2 );

			if ( d < best_distance )
			{
				best_distance = d;
				obj_id = o;
			}
		}

		if ( obj_id != -1 )
		{
			Pipeline::instance()->getVirtualCameraController()->selectObjectForTracking( obj_id );
		}
	}
}





