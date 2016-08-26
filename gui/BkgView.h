
#ifndef BKG_VIEW_H
#define BKG_VIEW_H
#pragma once



/////////////////////////////////////////////////////////////////////////////
// CBkgView view

class CBkgView : public CView
{
protected:

	CBkgView();           // protected constructor used by dynamic creation
	
	DECLARE_DYNCREATE(CBkgView)


// Attributes
protected:


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBkgView)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL


// Implementation
protected:
	
	virtual ~CBkgView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	// Generated message map functions
protected:
	
	//{{AFX_MSG(CBkgView)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
