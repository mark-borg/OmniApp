
#ifndef FLOATING_PANORAMA_VIEW_H
#define FLOATING_PANORAMA_VIEW_H
#pragma once


#include "FloatingImageView.h"
#include "..\core\pipeline.h"




class CFloatingPanoramaView : public CFloatingImageView
{
	typedef Pipeline::PanoramicDewarper_Type	dewarper_type;

	int token;			// token to identify the pipeline node object


	CPoint start_pt, end_pt;


	static const CPoint null_pt;


public:

	CFloatingPanoramaView( CWnd* pParent = NULL )
		:	CFloatingImageView( pParent )
	{
		start_pt = null_pt;
		token = -1;
	}

	
	CFloatingPanoramaView( CString title, CWnd* pParent = NULL )
		:	CFloatingImageView( title, pParent )
	{
		start_pt = null_pt;
	}


	void setNodeToken( int node_token )
	{
		assert( node_token != -1 );
		token = node_token;
	}


	bool canChangeView( CPoint point );


	bool changeView( CPoint point );


	virtual void CreateToolbar();


private:

	void displayInfo( dewarper_type* );


protected:

	// Generated message map functions
	//{{AFX_MSG(CFloatingPanoramaView)
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};




#endif 

