// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef MAIN_FRM_H
#define MAIN_FRM_H
#pragma once



class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();


protected: 
	
	DECLARE_DYNAMIC(CMainFrame)


// Attributes
public:


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL


// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


protected:
	
	CStatusBar		m_wndStatusBar;
	CToolBar		m_wndToolBar;
	CSplitterWnd	splitter1;
	CSplitterWnd	splitter2;


public:

	// if using the fast version for updating the status bar, then
	// ensure that the text pointer passed as argument to this function
	// is not a temporary object, but long-lived (for e.g. global or 
	// static type object). Reason is that the fast version uses a 
	// PostMessage command to the underlying window of the status bar
	// control. This just queues the message and the actual access to 
	// the text buffer for painting and screen updating will occur later.
	// The slow version uses a SendMessage command, which blocks until
	// the screen update is finished.
	void SetStatusBar( int pane, const char* text, bool fast_version = false )
	{
		CStatusBarCtrl& ctrl = m_wndStatusBar.GetStatusBarCtrl();

		if ( fast_version )
			::PostMessage( ctrl.m_hWnd, SB_SETTEXT, pane, (LPARAM)text );
		else
			ctrl.SetText( text, pane, 0 );
	}



	void doStatistics( IplData* buffer );


// Generated message map functions
protected:
	
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnWindowsCascade();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif 
