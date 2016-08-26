// OmniApp2.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "OmniApp2.h"
#include "MainFrm.h"
#include "..\core\pipeline.h"
#include "..\tools\MemInfo.h"
#include "..\tools\pview\pview.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMainApp

BEGIN_MESSAGE_MAP(CMainApp, CWinApp)
	//{{AFX_MSG_MAP(CMainApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_PROCESSING_PLAY, OnStartPipeline)
	ON_COMMAND(ID_PROCESSING_PAUSE, OnPausePipeline)
	ON_COMMAND(ID_DUMP_MEMORY_POOL, OnDumpMemoryPool)
	ON_COMMAND(ID_CLEAR_TRACE_WINDOW, OnClearTraceWindow)
	ON_COMMAND(ID_SAVE_TRACE, OnSaveTrace)
	ON_COMMAND(ID_DUMP_THREAD_INFO, OnDumpThreadInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


#include <winbase.h>
/////////////////////////////////////////////////////////////////////////////
// CMainApp construction

CMainApp::CMainApp()
{
	bkg_view = 0;
	log_view = 0;
	cam_view = 0;

	splash_win_timer = 0;

	SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS );
}


CMainApp::~CMainApp()
{
}



/////////////////////////////////////////////////////////////////////////////
// The one and only CMainApp object

CMainApp theApp;



/////////////////////////////////////////////////////////////////////////////
// CMainApp initialization

BOOL CMainApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.
	CMainFrame* pFrame = new CMainFrame;
	m_pMainWnd = pFrame;


	// show splash screen
	if ( splash_win.Create( m_pMainWnd ) )
	{
		splash_win.ShowWindow( SW_SHOW );
		splash_win.UpdateWindow();
		splash_win_timer = splash_win.SetTimer( WM_USER + 1, CSplashWindow::TIME_TO_LIVE, NULL );
	}


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine( cmdInfo );
	if ( ! cmdInfo.m_strFileName.IsEmpty() )
	{
		// make full path to avoid Windows using the path C:\WINNT 
		char full_name[_MAX_PATH];
		CfgParameters::setParamFilename( _fullpath( full_name, cmdInfo.m_strFileName, _MAX_PATH ) );
	}


	// do initialisation here...
	Sleep( 500 );


	// create and load the frame with its resources
	pFrame->LoadFrame( IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL );


	// set app icons
	HICON icon = LoadIcon( IDR_SMALL_APP_ICON );
	m_pMainWnd->SetIcon( icon, false );
	icon = LoadIcon( IDR_LARGE_APP_ICON );
	m_pMainWnd->SetIcon( icon, true );

	
	LOG( Msg::INIT_READY );


	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow( SW_SHOWMAXIMIZED );
	pFrame->UpdateWindow();


	// used by pview tools when displaying thread information
	registerThreadName( AfxGetThread()->m_nThreadID, "main" );


	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CMainApp message handlers


// App command to run the dialog
void CMainApp::OnAppAbout()
{
	if ( splash_win.Create( m_pMainWnd ) )
	{
		splash_win.ShowWindow( SW_SHOW );
		splash_win.UpdateWindow();
	}
}



BOOL CMainApp::PreTranslateMessage( MSG* pMsg ) 
{
	BOOL bResult = CWinApp::PreTranslateMessage( pMsg );

	if ( pMsg->message == IDC_CREATE_VIEW3 )
	{
		cam_view->autoCreateWindow();
	}
	if ( pMsg->message == ID_SHOW_PERSPECTIVE_WINDOWS )
	{
		cam_view->showPerspectiveWindows();
	}


	if ( splash_win.m_hWnd != NULL &&
		( ( pMsg->message == WM_TIMER && splash_win_timer == pMsg->wParam ) ||
		  pMsg->message == WM_KEYDOWN ||
		  pMsg->message == WM_SYSKEYDOWN ||
		  pMsg->message == WM_LBUTTONDOWN ||
		  pMsg->message == WM_RBUTTONDOWN ||
		  pMsg->message == WM_MBUTTONDOWN ||
		  pMsg->message == WM_NCLBUTTONDOWN ||
		  pMsg->message == WM_NCRBUTTONDOWN ||
		  pMsg->message == WM_NCMBUTTONDOWN ) )
	{
		if ( splash_win_timer != 0 )
			splash_win.KillTimer( splash_win_timer );
		splash_win.DestroyWindow();
		m_pMainWnd->UpdateWindow();

		return FALSE;
	}

	return bResult;
}




void CMainApp::OnStartPipeline() 
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	// TO DO: temporary measure: so far it's not safe to restart
	// the pipeline. windows are still attached to the old objects.
	// must clean these first, before creating a new pipeline
	if ( Pipeline::instance()->getReturnCode() != STILL_ACTIVE )
		return;

	if ( Pipeline::instance()->safe_state.Lock( 500 ) )
	{
		if ( Pipeline::instance()->getReturnCode() != STILL_ACTIVE )
		{
		}

		if ( ! Pipeline::instance()->hasStarted() ||
			 Pipeline::instance()->isSuspended() ||
			 Pipeline::instance()->getReturnCode() != STILL_ACTIVE )
		{
			// kill pipeline, so we can re-create it further below
			if ( Pipeline::instance()->getReturnCode() != STILL_ACTIVE )
			{
				( (CMainApp*) AfxGetApp() )->cam_view->closeFloatingWindows();
				Pipeline::instance()->safe_state.Unlock();
				Pipeline::killInstance();
				Pipeline::instance()->safe_state.Lock();
			}

			DWORD rc = Pipeline::instance()->resume();		

			if ( rc != 0xFFFFFFFF )
			{
				main_tool_bar->GetToolBarCtrl().Indeterminate( ID_PROCESSING_PAUSE, FALSE );
				main_tool_bar->GetToolBarCtrl().Indeterminate( ID_PROCESSING_PLAY, TRUE );
			}
		}
	
		Pipeline::instance()->safe_state.Unlock();
	}
}




void CMainApp::OnPausePipeline() 
{
	if ( Pipeline::instance()->isInitialising() )
		return;

	if ( Pipeline::instance()->safe_state.Lock( 500 ) )
	{
		if ( Pipeline::instance()->hasStarted() &&
			 ! Pipeline::instance()->isSuspended() )
		{
			DWORD rc = Pipeline::instance()->suspend();

			if ( rc != 0xFFFFFFFF )
			{
				main_tool_bar->GetToolBarCtrl().Indeterminate( ID_PROCESSING_PAUSE, TRUE );
				main_tool_bar->GetToolBarCtrl().Indeterminate( ID_PROCESSING_PLAY, FALSE );
			}
		}

		Pipeline::instance()->safe_state.Unlock();
	}
}




void CMainApp::OnDumpMemoryPool() 
{
	showMemInfo();
	Pipeline::instance()->MemPool.dump();
}




void CMainApp::OnClearTraceWindow() 
{
	log_view->clear();
}




void CMainApp::OnSaveTrace() 
{
	log_view->saveToFile();
}




void CMainApp::OnDumpThreadInfo() 
{
	ostrstream strm;
	showProcessThreadInfo( strm, "OmniApp2" );
	strm << ends;

	LOG( "-----------------------------------------------------------" );
	LOG( strm.str() );
	LOG( "-----------------------------------------------------------" );

	strm.rdbuf()->freeze( 0 );
}





