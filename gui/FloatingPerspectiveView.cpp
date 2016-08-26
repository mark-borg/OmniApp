
#include "stdafx.h"
#include "FloatingPerspectiveView.h"
#include <cv.h>



BEGIN_MESSAGE_MAP( CFloatingPerspectiveView, CFloatingImageView )
	//{{AFX_MSG_MAP(CFloatingPerspectiveView)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_COMMAND( ID_SWAP_LEFT_RIGHT, OnSwapLeftRight )
	ON_COMMAND( ID_ZOOM_IN, OnZoomIn )
	ON_COMMAND( ID_ZOOM_OUT, OnZoomOut )
	ON_COMMAND( ID_ROLL_CAMERA, OnRollCamera )
	ON_COMMAND( ID_ROLL_CAMERA_CCW, OnRollCameraCcw )
	ON_COMMAND(ID_PAN_CAMERA_LEFT, OnPanCameraLeft)
	ON_COMMAND(ID_PAN_CAMERA_RIGHT, OnPanCameraRight)
	ON_COMMAND(ID_TILT_CAMERA_DOWN, OnTiltCameraDown)
	ON_COMMAND(ID_TILT_CAMERA_UP, OnTiltCameraUp)
	ON_COMMAND(ID_LOCK_WINDOW, OnLockWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



const CPoint CFloatingPerspectiveView::null_pt = CPoint( -1, -1 );





void CFloatingPerspectiveView::CreateToolbar()
{
	if ( m_toolBar.Create( this ) )
	{
		const UINT cmds[] = {	ID_SAVE_PICTURE, 
								ID_SEPARATOR, 
								ID_SWAP_LEFT_RIGHT, 
								ID_SEPARATOR, 
								ID_ZOOM_IN, 
								ID_ZOOM_OUT,
								ID_SEPARATOR,
								ID_PAN_CAMERA_LEFT,
								ID_PAN_CAMERA_RIGHT,
								ID_SEPARATOR,
								ID_TILT_CAMERA_UP,
								ID_TILT_CAMERA_DOWN,
								ID_SEPARATOR,
								ID_ROLL_CAMERA_CCW,
								ID_ROLL_CAMERA,
								ID_SEPARATOR,
								ID_LOCK_WINDOW };
		m_toolBar.LoadBitmap( IDR_FLOAT_WINDOW_TOOLBAR );
		m_toolBar.SetButtons( (const UINT*) &cmds, 17 );
	}
}




bool CFloatingPerspectiveView::canChangeView( CPoint point )
{
	bool res;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		res = dewarper && dewarper->hasStarted();
		
		Pipeline::instance()->safe_state.Unlock();
	}

	return res;
}




void CFloatingPerspectiveView::OnMouseMove( UINT nFlags, CPoint point ) 
{
	if ( nFlags == MK_LBUTTON && start_pt != null_pt )
	{
		SetCursor( LoadCursor( 0, IDC_SIZEALL ) );
		if ( changeView( point ) )
			start_pt = point - CPoint( origin_x + scroll_x, origin_y + scroll_y );
	}
	else if ( canChangeView( point ) )
		SetCursor( LoadCursor( 0, IDC_SIZEALL ) );

	CFloatingImageView::OnMouseMove( nFlags, point );
}




bool CFloatingPerspectiveView::changeView( CPoint point )
{
	bool changed = false;

	end_pt = point - CPoint( origin_x + scroll_x, origin_y + scroll_y );
	
	int dx = abs( end_pt.x - start_pt.x );
	int dy = abs( end_pt.y - start_pt.y );

	if ( dx > 3 || dy > 3 )			// at least some movement, to be worth re-doing the mapping
	{
		dx = end_pt.x - start_pt.x;
		dy = - end_pt.y + start_pt.y;

		
		Pipeline::instance()->safe_state.Lock();
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			double	pan_delta = dx * dewarper->getHFOV() / dewarper->getWidth();

			if ( dewarper->isLeftRight() )
				pan_delta = - pan_delta;


			double	tilt_delta = dy * dewarper->getVFOV() / dewarper->height();

			if ( ! dewarper->isInvertedCamera() )
				tilt_delta = - tilt_delta;


			dewarper->adjustPanDelta( pan_delta );
			dewarper->adjustTiltDelta( tilt_delta );
			dewarper->updateMapping();


			displayInfo( dewarper );
			changed = true;
		}

		Pipeline::instance()->safe_state.Unlock();
	}

	return changed;
}




void CFloatingPerspectiveView::OnLButtonDown( UINT nFlags, CPoint point ) 
{
	start_pt = null_pt;

	if ( canChangeView( point ) )
	{
		SetCursor( LoadCursor( 0, IDC_SIZEALL ) );
		start_pt = point - CPoint( origin_x + scroll_x, origin_y + scroll_y );
	}

	CFloatingImageView::OnLButtonDown(nFlags, point);
}




void CFloatingPerspectiveView::OnLButtonUp( UINT nFlags, CPoint point ) 
{
	if ( start_pt != null_pt && canChangeView( point ) )
	{
		changeView( point );
	}

	start_pt = null_pt;
	
	CFloatingImageView::OnLButtonUp( nFlags, point );
}




void CFloatingPerspectiveView::OnSwapLeftRight()
{
	if ( converter.empty() )
		return;


	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			dewarper->toggleLeftRight();
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



// max_zoom = xN, min_zoom = x(1/N)
#define MAX_ZOOM	8
#define MIN_ZOOM	4		



void CFloatingPerspectiveView::OnZoomIn() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;


	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			double new_zoom = dewarper->getZoom();
			if ( new_zoom >= 1 && new_zoom < MAX_ZOOM )
				++new_zoom;
			else if ( new_zoom < 1 )
				new_zoom = new_zoom * 2;

			dewarper->adjustZoom( new_zoom );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



void CFloatingPerspectiveView::OnZoomOut() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			double new_zoom = dewarper->getZoom();
			if ( new_zoom > 1 )
				--new_zoom;
			else if ( new_zoom <= 1 && new_zoom > (1.0/MIN_ZOOM) )
				new_zoom = new_zoom / 2;
			
			dewarper->adjustZoom( new_zoom );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



void CFloatingPerspectiveView::displayInfo( dewarper_type* dewarper )
{
	char buffer[100];
	
	float z = dewarper->getZoom();

	if ( z < 1 )
		sprintf( buffer, "zoom=x1/%1.1f  fov=%2.0f°x%2.0f°  pan=%2.0f°  tilt=%2.0f°", 
				 1 / z, 
				 dewarper->getHFOV(), 
				 dewarper->getVFOV(),
				 dewarper->getPan(),
				 dewarper->getTilt() );
	else
		sprintf( buffer, "zoom=x%1.1f  fov=%2.0f°x%2.0f°  pan=%2.0f°  tilt=%2.0f°", 
				 z, 
				 dewarper->getHFOV(), 
				 dewarper->getVFOV(),
				 dewarper->getPan(),
				 dewarper->getTilt() );

	setStatusText( buffer );
}



#define CAMERA_ROLL_STEP	10
#define CAMERA_PAN_STEP		20
#define CAMERA_TILT_STEP	10



void CFloatingPerspectiveView::OnRollCamera() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			bool inverted = CfgParameters::instance()->isCameraInverted();

			dewarper->adjustRollDelta( inverted ? - CAMERA_ROLL_STEP : CAMERA_ROLL_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}




void CFloatingPerspectiveView::OnRollCameraCcw() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			bool inverted = CfgParameters::instance()->isCameraInverted();

			dewarper->adjustRollDelta( inverted ? CAMERA_ROLL_STEP : - CAMERA_ROLL_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}




void CFloatingPerspectiveView::OnPanCameraLeft() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			dewarper->adjustPanDelta( -CAMERA_PAN_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



void CFloatingPerspectiveView::OnPanCameraRight() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			dewarper->adjustPanDelta( CAMERA_PAN_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



void CFloatingPerspectiveView::OnTiltCameraDown() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			bool inverted = CfgParameters::instance()->isCameraInverted();

			dewarper->adjustTiltDelta( inverted ? CAMERA_TILT_STEP : - CAMERA_TILT_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}



void CFloatingPerspectiveView::OnTiltCameraUp() 
{
	if ( converter.empty() )
		return;

	if ( ! canChangeView( null_pt ) )
		return;

	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );

		if ( dewarper )
		{
			bool inverted = CfgParameters::instance()->isCameraInverted();

			dewarper->adjustTiltDelta( inverted ? - CAMERA_TILT_STEP : CAMERA_TILT_STEP );
			dewarper->updateMapping();

			displayInfo( dewarper );
		}
	}
	Pipeline::instance()->safe_state.Unlock();
}




int CFloatingPerspectiveView::getStatusBarSize()
{
	CDC* dc = GetDC();

	CSize sz = dc->GetTextExtent( "M", 1 );

	ReleaseDC( dc );

	return sz.cy;
}




void CFloatingPerspectiveView::update( const IplData image_data )
{
	CFloatingImageView::update( image_data );


	// display status text
	if ( ! flip_flop-- )
	{
		Pipeline::instance()->safe_state.Lock();
		{
			dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );
			displayInfo( dewarper );
		}
		Pipeline::instance()->safe_state.Unlock();
		
		flip_flop = 5;
	}
}




void CFloatingPerspectiveView::OnLockWindow() 
{
	Pipeline::instance()->safe_state.Lock();
	{
		dewarper_type* dewarper = Pipeline::instance()->getPerspectiveNode( token );
		dewarper->toggleManualLock();
	}
	Pipeline::instance()->safe_state.Unlock();
}




