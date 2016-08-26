// SplashWindow.cpp : implementation file
//

#include "stdafx.h"
#include "omniapp2.h"
#include "SplashWindow.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CSplashWindow dialog

#include "..\core\Version.h"



CSplashWindow::CSplashWindow(CWnd* pParent /*=NULL*/)
	: CDialog(CSplashWindow::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSplashWindow)
	m_version = _T( Version::instance()->getVersion() );
	m_build = _T( Version::instance()->getBuild() );
	m_libraries = _T( Version::instance()->getLibraries() );
	//}}AFX_DATA_INIT
}



void CSplashWindow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSplashWindow)
	DDX_Text(pDX, IDC_SPLASH_VERSION, m_version);
	DDX_Text(pDX, IDC_SPLASH_BUILD, m_build);
	DDX_Text(pDX, IDC_SPLASH_LIBRARIES, m_libraries);
	//}}AFX_DATA_MAP
}



BEGIN_MESSAGE_MAP(CSplashWindow, CDialog)
	//{{AFX_MSG_MAP(CSplashWindow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////

BOOL CSplashWindow::Create(CWnd* pParent)
{
	if (!CDialog::Create(CSplashWindow::IDD, pParent))
	{
		TRACE0("Warning: creation of CSplashWindow dialog failed\n");
		return FALSE;
	}

	return TRUE;
}



BOOL CSplashWindow::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CenterWindow();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


