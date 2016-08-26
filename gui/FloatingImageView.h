
#ifndef FLOATING_IMAGE_VIEW_H
#define FLOATING_IMAGE_VIEW_H
#pragma once


#include "resource.h"
#include "Ipl2Bmp.h"
#include "..\framework\RunnableObject.h"
#include "..\framework\IplData.h"



/////////////////////////////////////////////////////////////////////////////
// CImageView dialog


class CFloatingImageView : public CDialog, public RunnableObject
{
protected:

	CString		title;
	CToolBar    m_toolBar;
	int			origin_x, origin_y;		// top-left corner of client area (because of toolbar)
	
	int			scroll_x, scroll_y;		// the current scrolling amount
	int			scroll_dx, scroll_dy;	// scroll range

	Ipl2Bmp		converter;
	bool		first_time;
	HANDLE		do_paint;

	int			current_frame;			// keep current frame number

	CString		status_text;


public:

	CFloatingImageView( CWnd* pParent = NULL )
	{
		CFloatingImageView( "", pParent );
		status_text = "";
	}

	
	CFloatingImageView( CString title, CWnd* pParent = NULL );


	void setName( CString title )
	{
		this->title = title;
	}


	CString getName()
	{
		return title;
	}


	virtual ~CFloatingImageView()
	{
		CloseHandle( do_paint );
	}


	bool Create( CWnd* parent )
	{
		bool rc = CDialog::Create( IDD_FLOATING_WINDOW, parent ) == TRUE;
		
		assert( m_hWnd );
		resume();					// we start the thread once the window is created

		return rc;
	}


	void resizeToImage();


	virtual int getStatusBarSize()
	{
		return 0;		// no status bar for the base class
	}

	
	virtual void update( const IplData image_data );


	UINT loop();


	void reset()
	{
		converter.reset();

		first_time = true;
		scroll_x = scroll_y = 0;
		scroll_dx = scroll_dy = 0;
	}


	void ValidateToolbarRegion();


	virtual void CreateToolbar();


	void setStatusText( CString str )
	{
		status_text = str;
	}


// Dialog Data
	//{{AFX_DATA(CFloatingImageView)
	enum { IDD = IDD_FLOATING_WINDOW };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFloatingImageView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFloatingImageView)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnClose();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnCloseFloatWindow();
	afx_msg void OnSavePicture();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	
	void showSaveWarning();

	virtual void customisePopupMenu( CMenu* pMenu );

};




#endif 

