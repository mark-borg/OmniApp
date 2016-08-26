// ImageView.cpp : implementation file
//

#include "stdafx.h"
#include "FloatingImageView.h"
#include "..\core\Pipeline.h"
#include "..\tools\pview\pview.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CImageView dialog


CFloatingImageView::CFloatingImageView( CString title, CWnd* pParent /*=NULL*/ )
	: CDialog( CFloatingImageView::IDD, pParent )
{
	//{{AFX_DATA_INIT(CFloatingImageView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	this->title = title;
	
	first_time = true;

	do_paint = CreateEvent(	NULL, 
							false,		// automatic reset
							false,		// initially NOT signalled
							NULL );

	scroll_x = scroll_y = 0;
	scroll_dx = scroll_dy = 0;


	// used by pview tools when displaying thread information
	CString thread_name = "WND: " + title;
	registerThreadName( this->threadID(), thread_name.operator LPCTSTR() );
}



void CFloatingImageView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFloatingImageView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(CFloatingImageView, CDialog)
	//{{AFX_MSG_MAP(CFloatingImageView)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CLOSE()
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_CLOSE_FLOAT_WINDOW, OnCloseFloatWindow)
	ON_COMMAND(ID_SAVE_PICTURE, OnSavePicture)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()




/////////////////////////////////////////////////////////////////////////////
// CImageView message handlers

BOOL CFloatingImageView::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CreateToolbar();

	DWORD style = m_toolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;
	style ^= CBRS_BORDER_TOP;
	m_toolBar.SetBarStyle( style );

	// We need to resize the dialog to make room for control bars.
	// First, figure out how big the control bars are.
	CRect rcClientStart;
	CRect rcClientNow;
	GetClientRect(rcClientStart);
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,
				   0, reposQuery, rcClientNow);

	// Now move all the controls so they are in the same relative
	// position within the remaining client area as they would be
	// with no control bars.
	CPoint ptOffset(rcClientNow.left - rcClientStart.left,
					rcClientNow.top - rcClientStart.top);

	CRect  rcChild;
	CWnd* pwndChild = GetWindow(GW_CHILD);
	while (pwndChild)
	{
		pwndChild->GetWindowRect(rcChild);
		ScreenToClient(rcChild);
		rcChild.OffsetRect(ptOffset);
		pwndChild->MoveWindow(rcChild, FALSE);
		pwndChild = pwndChild->GetNextWindow();
	}

	// Adjust the dialog window dimensions
	CRect rcWindow;
	GetWindowRect(rcWindow);
	rcWindow.right += origin_x = ( rcClientStart.Width() - rcClientNow.Width() );
	rcWindow.bottom += origin_y = ( rcClientStart.Height() - rcClientNow.Height() );
	MoveWindow(rcWindow, FALSE);

	// And position the control bars
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);


	SetWindowText( title );


	return TRUE;  // return TRUE unless you set the focus to a control
}




void CFloatingImageView::CreateToolbar()
{
	// Create toolbar at the top of the dialog window
	if (m_toolBar.Create(this))
	{
		const UINT cmds[] = { ID_SAVE_PICTURE };
		m_toolBar.LoadBitmap( IDR_FLOAT_WINDOW_TOOLBAR );
		m_toolBar.SetButtons( (const UINT*) &cmds, 1 );

		//CToolBarCtrl& ctrl = m_toolBar.GetToolBarCtrl();
		//ctrl.SetState( ID_FLOAT_STOP, TBSTATE_INDETERMINATE );
	}
}




void CFloatingImageView::update( const IplData image_data )
{
	converter.update( image_data.img );
	current_frame = image_data.frame;

	if ( first_time )
	{
		resizeToImage();
		first_time = false;
	}

	if ( WaitForSingleObject( do_paint, 0 ) != WAIT_OBJECT_0 )
		SetEvent( do_paint );
}




void CFloatingImageView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CRect rectClient;
	GetClientRect( &rectClient );

	if ( converter.empty() )
	{
		dc.FillSolidRect( rectClient, GetSysColor( COLOR_3DFACE ) );
		return;
	}


	converter.paint( dc.m_hDC, origin_x + scroll_x, origin_y + scroll_y );


 	// since we are not painting the background to eliminate flickering, we must erase the extra parts
	if ( rectClient.right > converter.width() + scroll_x )
	{
		CRect xtr = rectClient;
		xtr.left = converter.width() + origin_x + scroll_x;
		xtr.bottom = converter.height() + origin_y;
		dc.FillSolidRect( xtr, GetSysColor( COLOR_3DFACE ) );
	}

	if ( rectClient.bottom > converter.height() + scroll_y )
	{
		CRect xtr = rectClient;
		xtr.top = converter.height() + origin_y + scroll_y;
		dc.FillSolidRect( xtr, GetSysColor( COLOR_3DFACE ) );
	}
	

	// any status text to display?
	if ( ! status_text.IsEmpty() )
		dc.TextOut( 1, converter.height() + origin_y + scroll_y, status_text );


	// Do not call CDialog::OnPaint() for painting messages
}




void CFloatingImageView::resizeToImage()
{
	int dx = converter.empty() ? 320 : converter.width();
	int dy = converter.empty() ? 240 : converter.height() + getStatusBarSize();

	dx = MAX( 320, dx );	// because of width of toolbar and status bar

	CRect r;
	GetWindowRect(&r);

	scroll_x = scroll_y = 0;
	scroll_dx = scroll_dy = 0;

	MoveWindow( r.left , 
				r.top, 
				dx + origin_x + GetSystemMetrics( SM_CXHSCROLL ) + GetSystemMetrics( SM_CXSIZEFRAME )*2, 
				dy + origin_y + GetSystemMetrics( SM_CYVSCROLL ) + GetSystemMetrics( SM_CYSIZEFRAME )*2 + GetSystemMetrics( SM_CYCAPTION ) );
}




BOOL CFloatingImageView::OnEraseBkgnd(CDC* pDC) 
{
	// no background erasure (because of flickering); instead OnDraw() 
	// will handle the background parts not covered by the image.
	return TRUE;
}




void CFloatingImageView::OnClose() 
{
	// we don't really close the window, but just hide it and re-use it if
	// the user clicks again on the camera.
	ShowWindow( SW_HIDE );

	CfgParameters::instance()->saveWindowState( getName(), false );
}




UINT CFloatingImageView::loop()
{
	while( true )
	{
		if ( WaitForSingleObject( do_paint, A_LONG_TIME * 2 ) == WAIT_OBJECT_0 )
		{
			Invalidate();
			ValidateToolbarRegion();
			UpdateWindow();
		}

		checkIfMustStop( RC_ABORTED );		// told to stop?

		if ( m_hWnd == 0 )					// has window been deleted?
			terminate( RC_ABORTED );
	}
}




void CFloatingImageView::OnRButtonDown( UINT nFlags, CPoint point ) 
{
	CMenu popupMenu;
	customisePopupMenu( &popupMenu );

	// highlight item
	bool can_save = ! converter.empty() && Pipeline::instance()->isSuspended();
	popupMenu.EnableMenuItem( ID_SAVE_PICTURE, can_save ? MF_ENABLED : MF_GRAYED );

	if ( ! converter.empty() && ! can_save )
		showSaveWarning();

	// create popup menu
	ClientToScreen( &point );
	popupMenu.GetSubMenu(0)->TrackPopupMenu( TPM_LEFTALIGN, point.x, point.y, this );
}




void CFloatingImageView::customisePopupMenu( CMenu* pMenu )
{
	pMenu->LoadMenu( IDR_FLOAT_WINDOW_MENU );
}




void CFloatingImageView::showSaveWarning()
{
	static bool only_once = true;

	if ( only_once )
	{
		CLOG( Msg::SUSPEND_PROCESSING_TO_SAVE, 0x0000ff );
		only_once = false;
	}
}




void CFloatingImageView::OnCloseFloatWindow() 
{
	OnClose();
}




void CFloatingImageView::OnSavePicture() 
{
	if ( converter.empty() )
	{
		CLOG( "No image to save!", 0x0000ff );
		return;
	}

	if ( ! Pipeline::instance()->isSuspended() )
	{
		CLOG( Msg::SUSPEND_PROCESSING_TO_SAVE, 0x0000ff );
		return;
	}


	char filename[100];
	sprintf( filename, "%s_frame%03.03d.bmp", title, current_frame );

	CFileDialog dlg( FALSE, 
					 ".bmp", 
					 filename, 
					 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
					 NULL,
					 this );
	if ( dlg.DoModal() == IDCANCEL )
		return;


	BeginWaitCursor();
	{
		Invalidate();
		UpdateWindow();
		Sleep(500);

		CString path_name = dlg.GetPathName();
		IplImage* ipl = converter.reConvert();

		safe_highgui.Lock();
		{
			cvSaveImage( path_name, ipl );
		}
		safe_highgui.Unlock();

		iplDeallocate( ipl, IPL_IMAGE_ALL );

		LOG( Msg::IMAGE_SAVED << path_name.operator LPCTSTR() );
	}
	EndWaitCursor();
}




void CFloatingImageView::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	// adjusting scroll bars
	if ( converter.empty() )
	{
		SetScrollRange( SB_HORZ, 0, 0, FALSE );
		SetScrollPos( SB_HORZ, 0, FALSE );
		ShowScrollBar( SB_HORZ );			// scroll bars always shown
		return;
	}


	//-------- determine horizontal scrolling ------------------------------
	int imgw = converter.width() + origin_x;
	RECT r;
	GetClientRect( &r );
	scroll_dx = ( ( scroll_dx = imgw - r.right ) < 0 ? 0 : scroll_dx );

	if ( -scroll_x > scroll_dx && scroll_dx )
	{
		scroll_x = -scroll_dx;
		Invalidate();
		//ValidateToolbarRegion();		-- because of toolbar
		UpdateWindow();
	}

	SetScrollRange( SB_HORZ, 0, scroll_dx );
	if ( scroll_dx == 0 )
	{
		scroll_x = 0;
		SetScrollRange( SB_HORZ, 0, 0, FALSE );
		SetScrollPos( SB_HORZ, 0, FALSE );
		ShowScrollBar( SB_HORZ );			// scroll bars always shown

		Invalidate();
		ValidateToolbarRegion();
		UpdateWindow();
	}
	
	if ( scroll_x > scroll_dx )
	{
		scroll_x = scroll_dx;
		SetScrollPos( SB_HORZ, scroll_x );
	}


	//-------- determine vertical scrolling ------------------------------
	int imgh = converter.height() + origin_y;
	scroll_dy = ( ( scroll_dy = imgh - r.bottom ) < 0 ? 0 : scroll_dy );

	if ( -scroll_y > scroll_dy && scroll_dy )
	{
		scroll_y = -scroll_dy;
		Invalidate();
		ValidateToolbarRegion();
		UpdateWindow();
	}

	SetScrollRange( SB_VERT, 0, scroll_dy );
	if ( scroll_dy == 0 )
	{
		scroll_y = 0;
		SetScrollRange( SB_VERT, 0, 0, FALSE );
		SetScrollPos( SB_VERT, 0, FALSE );
		ShowScrollBar( SB_VERT );			// scroll bars always shown

		Invalidate();
		ValidateToolbarRegion();
		UpdateWindow();
	}
	
	if ( scroll_y > scroll_dy )
	{
		scroll_y = scroll_dy;
		SetScrollPos( SB_VERT, scroll_y );
	}
}




void CFloatingImageView::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar ) 
{
	if ( converter.empty() )
		return;

	if ( nSBCode == SB_THUMBPOSITION )
		scroll_x = -nPos;
	else if ( nSBCode == SB_LINELEFT )
		++scroll_x;
	else if ( nSBCode == SB_PAGELEFT )
		scroll_x += 20;
	else if ( nSBCode == SB_LINERIGHT )
		--scroll_x;
	else if ( nSBCode == SB_PAGERIGHT )
		scroll_x -= 20;
	else
		return;

	scroll_x = scroll_x > 0 ? 0 : -scroll_x > scroll_dx ? -scroll_dx : scroll_x;

	SetScrollPos( SB_HORZ, -scroll_x );
	
	Invalidate();
	ValidateToolbarRegion();
	UpdateWindow();
}




void CFloatingImageView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if ( converter.empty() )
		return;

	if ( nSBCode == SB_THUMBPOSITION )
		scroll_y = -nPos;
	else if ( nSBCode == SB_LINEUP )
		++scroll_y;
	else if ( nSBCode == SB_PAGEUP )
		scroll_y += 20;
	else if ( nSBCode == SB_LINEDOWN )
		--scroll_y;
	else if ( nSBCode == SB_PAGEDOWN )
		scroll_y -= 20;
	else
		return;

	scroll_y = scroll_y > 0 ? 0 : -scroll_y > scroll_dy ? -scroll_dy : scroll_y;

	SetScrollPos( SB_VERT, -scroll_y );
	
	Invalidate();
	ValidateToolbarRegion();
	UpdateWindow();
}




void CFloatingImageView::ValidateToolbarRegion()
{
	// we don't need to redraw the toolbar every time
	RECT r;
	m_toolBar.GetWindowRect( &r );
	ScreenToClient( &r );
	ValidateRect( &r );
}




void CFloatingImageView::OnMove( int x, int y ) 
{
	CDialog::OnMove(x, y);

	if ( IsWindowVisible() )
	{
		RECT r;
		GetWindowRect( &r );
		CfgParameters::instance()->saveWindowPos( getName(), r.left, r.top );
	}
}




