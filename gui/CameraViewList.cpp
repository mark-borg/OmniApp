// CameraViewList.cpp : implementation file
//

#include "stdafx.h"
#include "omniapp2.h"
#include "CameraViewList.h"
#include "..\core\pipeline.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define INITIAL_WIDTH	320
#define INITIAL_HEIGHT	200




/////////////////////////////////////////////////////////////////////////////
// CCameraViewList

IMPLEMENT_DYNCREATE(CCameraViewList, CFormView)


CCameraViewList::CCameraViewList()
	: CFormView( CCameraViewList::IDD )
{
	CBitmap brsh;
	brsh.LoadBitmap( BMP_BACKGROUND00 );

	m_brush.CreatePatternBrush( &brsh );

	screen_offset_count = 0;
	num_windows = 0;
	num_folders = 0;


	const int icon_x = (int)( GetSystemMetrics( SM_CXICON ) * 2.25 );
	const int icon_y = (int)( icon_x * 0.75 );

	for ( int k = 0; k < MAX_WINDOWS; ++k )
	{
		m_video_float[ k ] = 0;
		m_video_folder[ k ] = 0;
		m_video_icon[ k ] = 0;
		m_video_icon_size[ k ] = CSize( icon_x, icon_y );
	}

	for ( int k = 0; k < MAX_FOLDERS; ++k )
	{
		m_folder_level[ k ] = 0;
		m_folder_parent[ k ] = -1;
		m_enable_folder_button[ k ] = true;
	}
}



CCameraViewList::~CCameraViewList()
{
	for ( int k = 0; k < num_windows; ++k )
	{
		delete m_video_float[k];
		delete m_video_icon[k];
	}
}



void CCameraViewList::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCameraViewList)
	DDX_Control(pDX, IDC_VIDEO_LABEL, m_folder[0]);
	DDX_Control(pDX, IDC_VIDEO_LABEL2, m_folder[1]);
	DDX_Control(pDX, IDC_VIDEO_LABEL3, m_folder[2]);
	DDX_Control(pDX, IDC_VIDEO_LABEL4, m_folder[3]);
	DDX_Control(pDX, IDC_VIDEO_LABEL5, m_folder[4]);

	DDX_Control(pDX, IDC_CREATE_VIEW, m_folder_button[0]);
	DDX_Control(pDX, IDC_CREATE_VIEW2, m_folder_button[1]);
	DDX_Control(pDX, IDC_CREATE_VIEW3, m_folder_button[2]);
	DDX_Control(pDX, IDC_CREATE_VIEW4, m_folder_button[3]);
	DDX_Control(pDX, IDC_CREATE_VIEW5, m_folder_button[4]);

	if ( m_video_icon[0] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX,	 *(m_video_icon[0]) );
	if ( m_video_icon[1] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX2, *(m_video_icon[1]) );
	if ( m_video_icon[2] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX3, *(m_video_icon[2]) );
	if ( m_video_icon[3] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX4, *(m_video_icon[3]) );
	if ( m_video_icon[4] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX5, *(m_video_icon[4]) );
	if ( m_video_icon[5] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX6, *(m_video_icon[5]) );
	if ( m_video_icon[6] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX7, *(m_video_icon[6]) );
	if ( m_video_icon[7] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX8, *(m_video_icon[7]) );
	if ( m_video_icon[8] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX9, *(m_video_icon[8]) );
	if ( m_video_icon[9] != 0 )	DDX_Control(pDX, IDC_VIDEO_BOX10, *(m_video_icon[9]) );
	if ( m_video_icon[10] != 0 ) DDX_Control(pDX, IDC_VIDEO_BOX11, *(m_video_icon[10]) );
	if ( m_video_icon[11] != 0 ) DDX_Control(pDX, IDC_VIDEO_BOX12, *(m_video_icon[11]) );
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCameraViewList, CFormView)
	//{{AFX_MSG_MAP(CCameraViewList)
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_PAINT()
	ON_COMMAND( IDC_CREATE_VIEW, OnFolderButton1 )
	ON_COMMAND( IDC_CREATE_VIEW2, OnFolderButton2 )
	ON_COMMAND( IDC_CREATE_VIEW3, OnFolderButton3 )
	ON_COMMAND( IDC_CREATE_VIEW4, OnFolderButton4 )
	ON_COMMAND( IDC_CREATE_VIEW5, OnFolderButton5 )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CCameraViewList diagnostics

#ifdef _DEBUG
void CCameraViewList::AssertValid() const
{
	CFormView::AssertValid();
}


void CCameraViewList::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG



/////////////////////////////////////////////////////////////////////////////
// CCameraViewList message handlers


BOOL CCameraViewList::OnEraseBkgnd( CDC* pDC ) 
{
	// Save old brush.
	CBrush* pOldBrush = pDC->SelectObject( &m_brush );

	CRect rect;
	pDC->GetClipBox( &rect );     // Erase the area needed.

	pDC->PatBlt( rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY );

	pDC->SelectObject( pOldBrush );

	return TRUE;
}



void CCameraViewList::closeFloatingWindows()
{
	for ( int k = 0; k < num_windows; k++ )
	{
		if ( m_video_float[k]->m_hWnd != 0 )
		{
			m_video_float[k]->ShowWindow( SW_HIDE );
			m_video_float[k]->end( 0 );
		}

		m_video_icon[k]->end( 0 );
	}
}



void CCameraViewList::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	ClientToScreen( &point );

	for ( int k = 0; k < num_windows; ++k )
	{
		CRect r;
		m_video_icon[ k ]->GetWindowRect( &r );

		if ( r.PtInRect( point ) )
		{
			openFloatingWindow( k, true );
			CfgParameters::instance()->saveWindowState( m_video_float[ k ]->getName(), true );
		}
	}
	
	CFormView::OnLButtonDblClk( nFlags, point );
}




void CCameraViewList::showPerspectiveWindows()
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	for ( int k = 0; k < num_windows; ++k )
	{
		if ( m_video_float[ k ]->m_hWnd == 0 )
		{
			openFloatingWindow( k, true );	
		}
	}
}




void CCameraViewList::openFloatingWindow( int k, bool show_on_create )
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	if ( k < 0 || k > num_windows )
		return;


	// window not yet created?
	if ( m_video_float[ k ]->m_hWnd == 0 )
	{
		CMainApp* app = (CMainApp*) AfxGetApp();

		m_video_float[ k ]->Create( app->m_pMainWnd );	

		// initial size
		m_video_float[ k ]->MoveWindow( 0, 0, INITIAL_WIDTH, INITIAL_HEIGHT, false );

		// position image
		CString s = m_video_float[ k ]->getName();
		int wx, wy;
		if ( CfgParameters::instance()->readWindowPos( s, wx, wy ) )
		{
			RECT img_rect;
			m_video_float[ k ]->GetWindowRect( &img_rect );
			m_video_float[ k ]->MoveWindow(	wx, wy, 
											img_rect.right - img_rect.left +1, 
											img_rect.bottom - img_rect.top +1 );
		}
		else
		{
			RECT bkg_rect, img_rect;
			CWnd* wnd = app->bkg_view;
			wnd->GetWindowRect( &bkg_rect );
			m_video_float[ k ]->GetWindowRect( &img_rect );
			m_video_float[ k ]->MoveWindow(	bkg_rect.left + 50 * screen_offset_count, 
					 						bkg_rect.top + 50 * screen_offset_count, 
											img_rect.right - img_rect.left +1, 
											img_rect.bottom - img_rect.top +1 );
			++screen_offset_count;
		}

		m_video_float[ k ]->ShowWindow( show_on_create ? SW_SHOW : SW_HIDE );
	}
	else if ( ! m_video_float[ k ]->IsWindowVisible() )
	{
		m_video_float[ k ]->reset();
		m_video_float[ k ]->ShowWindow( SW_SHOW );
	}

	
	m_video_float[ k ]->SetFocus();		// enable focus of window to bring to front
	SetFocus();		// restore focus back to the main window
}



void CCameraViewList::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	// folders 
	for ( int k = 0; k < num_folders; ++k )
	{
		m_folder[k].ShowWindow( SW_SHOW );
		m_folder[k].SetWindowText( m_folder_name[k] );	
		m_folder_button[k].ShowWindow( m_enable_folder_button[ k ] ? SW_SHOW : SW_HIDE );
	}

	// video icons
	for ( int k = 0; k < num_windows; ++k )
		m_video_icon[k]->ShowWindow( SW_SHOW );

	
	CPaintDC dc( this );			// device context for painting
	drawHierarchy( dc );
	UpdateWindow();
}




void CCameraViewList::OnPaint() 
{
	CPaintDC dc( this );			// device context for painting

	CFormView::OnPaint();			// let dialog draw its contents

	// on a call to this function, we only need to re-paint the arrows,
	// the other stuff are all members of the underlying dialog box and
	// are automatically taken care of (painted) by the dialog box.
	refreshHierarchy( dc );
}




void CCameraViewList::drawArrow( CPaintDC& dc, CRect r1, CRect r2 )
{
	CPen pen( PS_SOLID, 3, 0xaaaaaa );
	CPen* old_pen = (CPen*) dc.SelectObject( pen );

	static const int offset = GetSystemMetrics( SM_CXHSCROLL );

	dc.MoveTo( CPoint( r1.left + offset/2, r1.top + offset ) );
	dc.LineTo( CPoint( r1.left + offset/2, r2.top + offset ) );
	dc.LineTo( CPoint( r2.left, r2.top + offset ) );
	dc.LineTo( CPoint( r2.left - offset/2, r2.top + offset/2 ) );
	dc.MoveTo( CPoint( r2.left, r2.top + offset ) );
	dc.LineTo( CPoint( r2.left - offset/2, r2.top + offset + offset/2 ) );
}




void CCameraViewList::drawHierarchy( CPaintDC& dc )
{
	// arrange folders and videos in hierarchical structure
	static const int vertical_offset = GetSystemMetrics( SM_CYCAPTION );
	static const int horiz_offset = (int)( GetSystemMetrics( SM_CXHSCROLL ) * 2 );
	
	int top = vertical_offset;

	for ( int k = 0; k < num_folders; ++k )
	{
		CRect r, newr;

		m_folder[k].GetWindowRect( &r );
		ScreenToClient( &r );

		newr.top = top;
		newr.left = horiz_offset * m_folder_level[k] + (int)(horiz_offset/2);
		newr.bottom = r.Height() + newr.top;
		newr.right = r.Width() + newr.left;

		top += newr.Height();

		m_folder[k].MoveWindow( &newr );

		// folder button
		m_folder_button[k].GetWindowRect( &r );

		newr.left = newr.right;
		newr.right = newr.left + r.Width();
		newr.bottom = r.Height() + newr.top;
		m_folder_button[k].MoveWindow( &newr );


		// link folder to parent
		int parent = m_folder_parent[k];
		if ( parent != -1 )
		{
			CRect pr;
			m_folder[ parent ].GetWindowRect( &pr );
			m_folder[ k ].GetWindowRect( &r );
			ScreenToClient( &pr );
			ScreenToClient( &r );

			drawArrow( dc, pr, r );
		}

		// any videos belonging to this folder?
		for ( int v = 0; v < num_windows; ++v )
		{
			if ( m_video_folder[v] == k )
			{
				m_video_icon[v]->ShowWindow( SW_SHOW );

				m_video_icon[v]->GetWindowRect( &r );
				ScreenToClient( &r );

				newr.top = top;
				newr.left = horiz_offset * (m_folder_level[k] +1);
				newr.bottom = m_video_icon_size[v].cy + newr.top;
				newr.right = m_video_icon_size[v].cx + newr.left;

				top += newr.Height();

				m_video_icon[v]->MoveWindow( &newr );
			}
		}

		top += vertical_offset;
	}
}




void CCameraViewList::refreshHierarchy( CPaintDC& dc )
{
	// arrange folders and videos in hierarchical structure
	static const int vertical_offset = GetSystemMetrics( SM_CYCAPTION );
	static const int horiz_offset = (int)( GetSystemMetrics( SM_CXHSCROLL ) * 2 );
	
	int top = vertical_offset;

	for ( int k = 0; k < num_folders; ++k )
	{
		CRect r;

		m_folder[k].GetWindowRect( &r );
		ScreenToClient( &r );

		top += r.Height();

		// link folder to parent
		int parent = m_folder_parent[k];
		if ( parent != -1 )
		{
			CRect pr;
			m_folder[ parent ].GetWindowRect( &pr );
			m_folder[ k ].GetWindowRect( &r );
			ScreenToClient( &pr );
			ScreenToClient( &r );

			drawArrow( dc, pr, r );
		}

		// any videos belonging to this folder?
		for ( int v = 0; v < num_windows; ++v )
		{
			if ( m_video_folder[v] == k )
				top += m_video_icon_size[v].cy;
		}

		top += vertical_offset;
	}
}




int CCameraViewList::newFolder( CString name, int level, int parent, bool enable_button )
{
	assert( num_folders < MAX_FOLDERS );
	
	m_folder_name[ num_folders ] = name;	
	m_folder_level[ num_folders ] = level;
	m_folder_parent[ num_folders ] = parent;
	m_enable_folder_button[ num_folders ] = enable_button;
	
	++num_folders;

	return num_folders -1;
}




int CCameraViewList::newVideoWindow( CStaticImageView* icon_control, 
									 CFloatingImageView* popup_window,
									 int folder, 
									 float width_scale, 
									 float height_scale )
{
	if ( num_windows >= MAX_WINDOWS )
		return -1;
	
	m_video_float[ num_windows ] = popup_window;	
	m_video_icon[ num_windows ] = icon_control;
	m_video_folder[ num_windows ] = folder;
	m_video_icon_size[ num_windows ] = 
			CSize(	(int)( m_video_icon_size[ num_windows ].cx * width_scale ),
					(int)( m_video_icon_size[ num_windows ].cy * height_scale ) );

	++num_windows;

	return num_windows -1;
}




void CCameraViewList::OnFolderButton1()
{
}




void CCameraViewList::OnFolderButton2()
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	// re-direct action to pipeline object
	Pipeline::instance()->OnFolderButton2();
}




void CCameraViewList::OnFolderButton3()
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	// re-direct action to pipeline object
	Pipeline::instance()->OnNewPerspectiveView();
}




void CCameraViewList::autoCreateWindow()
{
	while ( Pipeline::instance()->isInitialising() )
	{
		// pipeline is initialising - wait a bit.
		// (some nodes of the pipeline are a bit slow to initialise, e.g. the 
		// HistoryNode has to call the OS command to remove files and create
		// directories.
		Sleep( 500 );
	}

	// re-direct action to pipeline object
	Pipeline::instance()->OnNewPerspectiveView();
}



void CCameraViewList::OnFolderButton4()
{
}




void CCameraViewList::OnFolderButton5()
{
}




