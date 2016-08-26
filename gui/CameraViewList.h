#ifndef CAMERA_VIEW_LIST_H
#define CAMERA_VIEW_LIST_H
#pragma once



/////////////////////////////////////////////////////////////////////////////
// CCameraViewList form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif
#include "StaticImageView.h"
#include "FloatingImageView.h"
#include "FloatingOmniImageView.h"
#include "FloatingPerspectiveView.h"
#include "FloatingPanoramaView.h"
#include "resource.h"



#define MAX_WINDOWS		12
#define MAX_FOLDERS		5



class CCameraViewList : public CFormView
{
public:


	int newFolder( CString name, int level, int parent = -1, bool enable_button = true );

	int newVideoWindow( CStaticImageView* icon_control, 
						CFloatingImageView* popup_window,
						int folder, 
						float width_scale = 1, 
						float height_scale = 1 );


	CStaticImageView* getVideoWindowIcon( int num )
	{
		assert( num >= 0 && num < num_windows );
		return m_video_icon[ num ];
	}


	CFloatingImageView* getVideoWindow( int num )
	{
		assert( num >= 0 && num < num_windows );
		return m_video_float[ num ];
	}


	int numWindows()
	{
		return num_windows;
	}


	void closeFloatingWindows();


	void openFloatingWindow( int k, bool show_on_create = false );


	void showPerspectiveWindows();



protected:
	
	CCameraViewList();           // protected constructor used by dynamic creation


	DECLARE_DYNCREATE( CCameraViewList )



private:
	
	CBrush m_brush;


	//{{AFX_DATA(CCameraViewList)
	enum { IDD = IDD_CAMERA_VIEW_LIST };
	CStatic				m_folder[ MAX_FOLDERS ];
	CButton				m_folder_button[ MAX_FOLDERS ];
	CStaticImageView*	m_video_icon[ MAX_WINDOWS ];
	//}}AFX_DATA

	int	m_folder_level[ MAX_FOLDERS ];
	int m_folder_parent[ MAX_FOLDERS ];
	CString m_folder_name[ MAX_FOLDERS ];
	bool m_enable_folder_button[ MAX_FOLDERS ];

	CFloatingImageView*	m_video_float[ MAX_WINDOWS ];
	int m_video_folder[ MAX_WINDOWS ];
	CSize m_video_icon_size[ MAX_WINDOWS ];				// in dialog units

	int num_windows;
	int num_folders;
	int screen_offset_count;		// used for popup window screen placement on its creation


	void drawArrow( CPaintDC& dc, CRect r1, CRect r2 );
	void drawHierarchy( CPaintDC& dc );
	void refreshHierarchy( CPaintDC& dc );


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCameraViewList)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	virtual ~CCameraViewList();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif


	// Generated message map functions
	//{{AFX_MSG(CCameraViewList)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnFolderButton1();
	afx_msg void OnFolderButton2();
	afx_msg void OnFolderButton3();
	afx_msg void OnFolderButton4();
	afx_msg void OnFolderButton5();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


	void autoCreateWindow();


	friend class CMainApp;
};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.


#endif 

