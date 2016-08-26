
#ifndef FLOATING_OMNI_IMAGE_VIEW_H
#define FLOATING_OMNI_IMAGE_VIEW_H
#pragma once


#include "FloatingImageView.h"
#include "..\core\pipeline.h"




class CFloatingOmniImageView : public CFloatingImageView
{

	typedef Pipeline::PanoramicDewarper_Type	dewarper_type1;
	typedef Pipeline::PerspectiveDewarper_Type	dewarper_type;

	bool borders_enabled;

	CPoint mouse_down_point;


public:

	CFloatingOmniImageView( CWnd* pParent = NULL )
		:	CFloatingImageView( pParent )
	{
		borders_enabled = true;
		
		mouse_down_point.x = -1;
		mouse_down_point.y = -1;
	}

	
	CFloatingOmniImageView( CString title, CWnd* pParent = NULL )
		:	CFloatingImageView( title, pParent )
	{
	}


	virtual void update( const IplData image_data );


	void DisableBorders()
	{
		borders_enabled = false;
	}


	void EnableBorders()
	{
		borders_enabled = true;
	}



protected:

	
	virtual void CreateToolbar();

	
	void drawBorders( const IplData image_data );


	virtual void customisePopupMenu( CMenu* pMenu );


	void OnChangeViewPosition( int window_id );



	// Generated message map functions
	//{{AFX_MSG(CFloatingOmniImageView)
	afx_msg void OnToggleBorders();
	afx_msg void OnPerspective1();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnPerspective2();
	afx_msg void OnPerspective3();
	afx_msg void OnPerspective4();
	afx_msg void OnPerspective5();
	afx_msg void OnPerspective6();
	afx_msg void OnTrackObject();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};




#endif 

