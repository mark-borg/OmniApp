// OmniApp2.h : main header file for the OMNIAPP2 application
//

#ifndef OMNI_APP2_H
#define OMNI_APP2_H
#pragma once


#include "stdafx.h"
#include "resource.h"       // main symbols
#include "SplashWindow.h"
#include "LogView.h"
#include "CameraViewList.h"
#include "BkgView.h"




/////////////////////////////////////////////////////////////////////////////
// CMainApp:
// See OmniApp2.cpp for the implementation of this class
//

class CMainApp : public CWinApp
{
public:
	CMainApp();

	~CMainApp();

	CSplashWindow		splash_win;
	UINT_PTR			splash_win_timer;
	CBkgView*			bkg_view;
	CLogView*			log_view;
	CCameraViewList*	cam_view;

	CStatusBar*			main_status_bar;
	CToolBar*			main_tool_bar;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation

public:
	//{{AFX_MSG(CMainApp)
	afx_msg void OnAppAbout();
	afx_msg void OnStartPipeline();
	afx_msg void OnPausePipeline();
	afx_msg void OnDumpMemoryPool();
	afx_msg void OnClearTraceWindow();
	afx_msg void OnSaveTrace();
	afx_msg void OnDumpThreadInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif 
