// LogView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogView view


#ifndef LOG_VIEW_H
#define LOG_VIEW_H
#pragma once


#include <afxrich.h>



class CLogView : public CRichEditView
{
	DECLARE_DYNCREATE( CLogView )

public:

	CLogView();           
	
	virtual ~CLogView();

	void AddMessage( CString msg, COLORREF color = 0xffffffff );

	void clear();

	void saveToFile();


#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

protected:

	//{{AFX_MSG(CLogView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////



#endif
