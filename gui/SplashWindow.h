
#ifndef SPLASH_WINDOW_H
#define SPLASH_WINDOW_H
#pragma once



/////////////////////////////////////////////////////////////////////////////
// CSplashWindow dialog

class CSplashWindow : public CDialog
{
// Construction
public:
	CSplashWindow(CWnd* pParent = NULL);   // standard constructor

	BOOL Create( CWnd* pParent );

	enum { TIME_TO_LIVE = 3500 };


// Dialog Data
	//{{AFX_DATA(CSplashWindow)
	enum { IDD = IDD_SPLASH_WINDOW };
	CString	m_version;
	CString	m_build;
	CString	m_libraries;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplashWindow)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSplashWindow)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif 
