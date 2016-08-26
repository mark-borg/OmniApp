
#ifndef FLOATING_PERSPECTIVE_VIEW_H
#define FLOATING_PERSPECTIVE_VIEW_H
#pragma once


#include "FloatingImageView.h"
#include "..\core\pipeline.h"




class CFloatingPerspectiveView : public CFloatingImageView
{
	typedef Pipeline::PerspectiveDewarper_Type	dewarper_type;

	int token;			// token to identify the pipeline node object

	CPoint start_pt, end_pt;
	
	char flip_flop;		// used to occasionally display camera info 

	static const CPoint null_pt;


public:

	CFloatingPerspectiveView( CWnd* pParent = NULL )
		:	CFloatingImageView( pParent )
	{
		start_pt = null_pt;
		token = -1;
		flip_flop = 1;
	}

	
	CFloatingPerspectiveView( CString title, CWnd* pParent = NULL )
		:	CFloatingImageView( title, pParent )
	{
		start_pt = null_pt;
		token = -1;
		flip_flop = 1;
	}


	void setNodeToken( int node_token )
	{
		assert( node_token != -1 );
		token = node_token;
	}


	int nodeToken()
	{
		return token;
	}


	bool canChangeView( CPoint point );


	bool changeView( CPoint point );


	virtual void CreateToolbar();

	
	virtual int getStatusBarSize();


	virtual void update( const IplData image_data );


private:


	void displayInfo( dewarper_type* );


protected:

	// Generated message map functions
	//{{AFX_MSG(CFloatingPerspectiveView)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSwapLeftRight();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnRollCamera();
	afx_msg void OnRollCameraCcw();
	afx_msg void OnPanCameraLeft();
	afx_msg void OnPanCameraRight();
	afx_msg void OnTiltCameraDown();
	afx_msg void OnTiltCameraUp();
	afx_msg void OnLockWindow();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};




#endif 

