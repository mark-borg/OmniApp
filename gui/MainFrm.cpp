// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "OmniApp2.h"
#include "MainFrm.h"
#include "CameraViewList.h"
#include "BkgView.h"
#include "LogView.h"
#include "..\core\pipeline.h"
#include "..\core\CfgParameters.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(ID_WINDOWS_CASCADE, OnWindowsCascade)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


static UINT indicators[] =
{
	ID_SEPARATOR,			// status line indicator
	ID_SEPARATOR,
	ID_SEPARATOR
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
}


CMainFrame::~CMainFrame()
{
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if ( CFrameWnd::OnCreate( lpCreateStruct ) == -1 )
		return -1;

	if ( ! m_wndToolBar.CreateEx( this, TBSTYLE_FLAT, 
								  WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | 
								  CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC ) ||
		 ! m_wndToolBar.LoadToolBar( IDR_MAINFRAME ) )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}


	if ( ! m_wndStatusBar.Create( this ) ||
		 ! m_wndStatusBar.SetIndicators( indicators, sizeof( indicators ) / sizeof( UINT ) ) )
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}


	// make toolbars dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	// set initial state of toolbar buttons
	m_wndToolBar.GetToolBarCtrl().Indeterminate( ID_PROCESSING_PAUSE );


	// status bar
	m_wndStatusBar.SetPaneInfo( 1, ID_SEPARATOR, SBPS_NORMAL, 200 );
	m_wndStatusBar.SetPaneInfo( 2, ID_SEPARATOR, SBPS_STRETCH, 200 );


	CMainApp* app = (CMainApp*) AfxGetApp();
	app->main_status_bar = &m_wndStatusBar;
	app->main_tool_bar = &m_wndToolBar;

	return 0;
}



BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG



/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CMainApp* app = (CMainApp*) AfxGetApp();

	
	if ( splitter1.CreateStatic( this, 1, 2 ) )
	{
		CRect rect;
		GetClientRect( &rect );

		CSize size = rect.Size();
		size.cx = size.cx / 5;

		if ( ! splitter1.CreateView( 0, 0, 
					RUNTIME_CLASS( CCameraViewList ), 
					size, pContext ) )
			return FALSE;


		if ( ! splitter2.CreateStatic(	&splitter1, 2, 1, 
										WS_CHILD | WS_VISIBLE | WS_BORDER,
										splitter1.IdFromRowCol( 0, 1 ) ) )
			return FALSE;
		
	

		if ( ! splitter2.CreateView( 0, 0, 
					RUNTIME_CLASS( CBkgView ), 
					size, pContext ) )
			return FALSE;		

		
		if ( ! splitter2.CreateView( 1, 0, 
					RUNTIME_CLASS( CLogView ), 
					size, pContext ) )
			return FALSE;



		// register views with main application class - these will be accessible for public use
		app->cam_view = (CCameraViewList*) splitter1.GetPane( 0, 0 );
		app->bkg_view = (CBkgView*) splitter2.GetPane( 0, 0 );
		app->log_view = (CLogView*) splitter2.GetPane( 1, 0 );


		return TRUE;
	}

	return FALSE;
}




void CMainFrame::OnClose() 
{
	if ( Pipeline::instance()->isInitialising() )
	{
		Beep( 400, 50 );
		return;
	}

	CDialog dlg;
	dlg.Create( IDD_CLOSURE_DLG, this );
	dlg.CenterWindow( this );
	dlg.ShowWindow( SW_SHOW );
	dlg.UpdateWindow();

	BeginWaitCursor();

	// close active floating windows first...
	( (CMainApp*) AfxGetApp() )->cam_view->closeFloatingWindows();

	// in case pipeline is still running...
	if ( Pipeline::instance()->isSuspended() )
		Pipeline::instance()->resume();				// resume thread so that it could end itself
	int rc = Pipeline::instance()->end( 10000 );	// wait for pipeline to terminate (at most 10 secs)

	Sleep( 500 );
	EndWaitCursor();
	
	if ( rc == -1 )
		return;

	CFrameWnd::OnClose();
}




void CMainFrame::doStatistics( IplData* buffer )
{
	assert( buffer );
	assert( buffer->img );

	// show some info in status bar

	// we use the fast version of the SetStatusBar() function, 
	// therefore the call to SetStatusBar() will not block until
	// the actual screen update has been done. Instead, it will
	// only place a message on the status bar control's queue to
	// tell it to update its text panes. This means that the 
	// status bar control will access the text buffer after we
	// leave this function, hence the static type for the buffers.
	static char text1[ 20 ], text2[ 40 ];
	
	sprintf( text1, "frame %d", buffer->frame );
	SetStatusBar( 1, text1, true );

	CfgParameters* params = CfgParameters::instance();
	long running_time = ( buffer->frame - params->getStartFrame() ) / params->getFps();
	sprintf( text2, "video stream time %d:%02.02d:%02.02d", 
			 running_time / 3600, ( running_time / 60 ) % 60, running_time % 60 );
	SetStatusBar( 2, text2, true );
}




void CMainFrame::OnWindowsCascade() 
{
	CMainApp* app = (CMainApp*) AfxGetApp();
	assert( app );
	
	int offset = 0;

	RECT bkg_rect, img_rect;
	CWnd* bwnd = app->bkg_view;
	bwnd->GetWindowRect( &bkg_rect );

	for ( int k = 0; k < app->cam_view->numWindows(); ++k )
	{
		CFloatingImageView* wnd = app->cam_view->getVideoWindow( k );
		if ( wnd->IsWindowVisible() )
		{
			wnd->GetWindowRect( &img_rect );
			wnd->MoveWindow( bkg_rect.left + 50 * offset, 
					 		 bkg_rect.top + 50 * offset, 
							 img_rect.right - img_rect.left +1, 
							 img_rect.bottom - img_rect.top +1 );
			++offset;
			wnd->SetFocus();
		}
	}

	bwnd->SetFocus();
}




