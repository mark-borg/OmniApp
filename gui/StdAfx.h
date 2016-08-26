// Microsoft Windows GUI common file
//

#ifndef STDAFX_H
#define STDAFX_H
#pragma once


#define VC_EXTRALEAN			// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>				// MFC core and standard components
#include <afxext.h>				// MFC extensions
#include <afxdisp.h>			// MFC Automation classes
#include <afxdtctl.h>			// MFC support for Internet Explorer 4 Common Controls
#include <afxmt.h>				// Windows Multithreading support
#ifndef _AFX_NO_AFXCMN_SUPPORT
#	include <afxcmn.h>			// MFC support for Windows Common Controls
#endif 


#include "..\core\log.h"
#include "..\tools\float_cast.h"
#include "..\tools\timer.h"



//----------------------- text --------------------------------------------------

#define MSG( ID, TEXT )		const char* const ID = TEXT;


namespace Msg
{
	MSG( SUSPEND_PROCESSING_TO_SAVE,		"Suspend processing if you want to save an image..." );
	MSG( INIT_READY,						"Application Initialisation ready." );
	MSG( CLICK_ICON_TO_SEE_LARGE_VIEW,		"Double click on one or more of the video stream icons to see a full-size view...\nThe icons are refreshed at a slower rate than the full-size views." );
	MSG( IMAGE_SAVED,						"Image has been saved successfully as: " );
	MSG( TRACE_SAVED,						"Contents of Trace window has been saved successfully as: " );
	MSG( ERROR_READING_SOURCE,				"No image source files were found. Please check the file path and file mask used." );
	MSG( PANORAMA_LIMIT_EXCEEDED,			"Exceeded Limit for the Number of Panoramic Views." );
	MSG( PERSPECTIVE_LIMIT_EXCEEDED,		"Exceeded Limit for the Number of Perspective Views." );
	MSG( TOTAL_WINDOW_LIMIT_EXCEEDED,		"Exceeded Total Limit for the Number of Views." );
	MSG( CALIBRATION_STARTED,				"Calibration Started." );
	MSG( CALIBRATION_FINISHED,				"Calibration Finished." );
}



#endif 
