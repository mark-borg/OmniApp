// BkgView.cpp : implementation file
//

#include "stdafx.h"
#include "omniapp2.h"
#include "BkgView.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CBkgView

IMPLEMENT_DYNCREATE(CBkgView, CView)

CBkgView::CBkgView()
{
}


CBkgView::~CBkgView()
{
}


BEGIN_MESSAGE_MAP(CBkgView, CView)
	//{{AFX_MSG_MAP(CBkgView)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBkgView drawing

void CBkgView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CBkgView diagnostics

#ifdef _DEBUG
void CBkgView::AssertValid() const
{
	CView::AssertValid();
}

void CBkgView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CBkgView message handlers

BOOL CBkgView::OnEraseBkgnd(CDC* pDC) 
{
   CBrush brush( GetSysColor( COLOR_APPWORKSPACE ) );

   // Save old brush.
   CBrush* pOldBrush = pDC->SelectObject( &brush );

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed.

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);

   pDC->SelectObject(pOldBrush);

   return TRUE;
}









