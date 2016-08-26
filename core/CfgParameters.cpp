
#include "CfgParameters.h"
#include "..\tools\CvUtils.h"
#include "..\gui\stdafx.h"
#include <cv.h>

 

#define GET_INT( VAR, PARAM, SECTION, DEFAULT )						\
						GetPrivateProfileString( SECTION,			\
												 PARAM,				\
												 DEFAULT,			\
												 tmp,				\
												 TMP_SIZE,			\
												 param_file_name );	\
						VAR = atoi( tmp );


#define GET_BOOL( VAR, PARAM, SECTION, DEFAULT )					\
						GetPrivateProfileString( SECTION,			\
												 PARAM,				\
												 DEFAULT,			\
												 tmp,				\
												 TMP_SIZE,			\
												 param_file_name );	\
						VAR = atoi( tmp ) > 0;


#define GET_FLT( VAR, PARAM, SECTION, DEFAULT )						\
						GetPrivateProfileString( SECTION,			\
												 PARAM,				\
												 DEFAULT,			\
												 tmp,				\
												 TMP_SIZE,			\
												 param_file_name );	\
						VAR = atof( tmp );


#define GET_STR( VAR, PARAM, SECTION, DEFAULT )						\
						GetPrivateProfileString( SECTION,			\
												 PARAM,				\
												 DEFAULT,			\
												 tmp,				\
												 TMP_SIZE,			\
												 param_file_name );	\
						VAR = tmp;


#define WRITE_INT( VAR, PARAM, SECTION )								\
						sprintf( tmp, "%d", VAR );						\
						WritePrivateProfileString( SECTION,				\
												   PARAM,				\
												   tmp,					\
												   param_file_name );	


#define WRITE_BOOL( VAR, PARAM, SECTION )								\
						 WritePrivateProfileString( SECTION,			\
												    PARAM,				\
													VAR ? "1" : "0",	\
													param_file_name );




CString CfgParameters::param_file_name = ".\\omnitracking.ini";

	


void CfgParameters::setParamFilename( CString param_file_name )
{
	CfgParameters::param_file_name = param_file_name;
}



CfgParameters::CfgParameters()
{
	mask = 0;
	azimuth_map = 0;
	altitude_map = 0;
	radial_map = 0;

	p_azimuth_map = 0;
	p_altitude_map = 0;
	p_radial_map = 0;


	// check if INI file exists or not
	FILE* param_file = fopen( param_file_name, "r" );
	if ( param_file == NULL )
	{
		AfxMessageBox( "The configuration file OMNITRACKING.INI was not found in the current directory.\n"
					   "Default Settings will be used instead!!" );
	}
	else
		fclose( param_file );


	LOG( "Using Initialisation file: " << (const char*) param_file_name );


	const int TMP_SIZE = 500;
	char tmp[ TMP_SIZE ];


	const char* OMNICAMERA			= "OMNICAMERA";
	const char* BKG_SUBTRACT		= "BACKGROUND_SUBTRACTION";
	const char* TRACKING			= "TRACKING";
	const char* BLOB_TRACKING		= "BLOB_TRACKING";
	const char* COLOUR_TRACKING		= "COLOUR_TRACKING";
	const char* CAMERA_CONTROLLER	= "CAMERA_CONTROLLER";
	const char* OUTPUT				= "OUTPUT";


	GET_BOOL( m_auto_calibrate,		"AUTO_CALIBRATION",	OMNICAMERA,		"1" );

	GET_STR( m_image_source,		"IMAGE_SEQUENCE",	OMNICAMERA,		"####.jpg" );
	GET_INT( m_start_frame,			"START_FRAME",		OMNICAMERA,		"1" );
	GET_INT( m_end_frame,			"END_FRAME",		OMNICAMERA,		"-1" );
	GET_INT( m_read_step,			"READ_STEP",		OMNICAMERA,		"1" );
	GET_INT( m_fps,					"FPS",				OMNICAMERA,		"15" );

	GET_INT( m_read_roi_min_x,		"READ_ROI_MIN_X",	OMNICAMERA,		"-1" );
	GET_INT( m_read_roi_max_x,		"READ_ROI_MAX_X",	OMNICAMERA,		"-1" );
	GET_INT( m_read_roi_min_y,		"READ_ROI_MIN_Y",	OMNICAMERA,		"-1" );
	GET_INT( m_read_roi_max_y,		"READ_ROI_MAX_Y",	OMNICAMERA,		"-1" );

	GET_INT( m_in_aspect_x,			"IN_ASPECT_X",		OMNICAMERA,		"1" );
	GET_INT( m_out_aspect_x,		"OUT_ASPECT_X",		OMNICAMERA,		"1" );
	GET_INT( m_in_aspect_y,			"IN_ASPECT_Y",		OMNICAMERA,		"1" );
	GET_INT( m_out_aspect_y,		"OUT_ASPECT_Y",		OMNICAMERA,		"1" );

	GET_INT( m_mirror_centre_x,		"IMAGE_CENTRE_X",	OMNICAMERA,		"160" );
	GET_INT( m_mirror_centre_y,		"IMAGE_CENTRE_Y",	OMNICAMERA,		"100" );
	GET_INT( m_boundary_radius,		"BOUNDARY_RADIUS",	OMNICAMERA,		"100" );
	GET_FLT( m_mirror_vfov,			"MIRROR_VFOV",		OMNICAMERA,		"90" );
	GET_BOOL( m_camera_inverted,	"INVERTED_CAMERA",	OMNICAMERA,		"0" );


	m_mask_circular_lower_bound = m_mirror_vfov - 0.5;	// avoid aliasing problems at mirror boundary
	sprintf( tmp, "%f", m_mask_circular_lower_bound );
	GET_FLT( m_mask_circular_lower_bound,	"DETECTION_LOWER_BOUND",	OMNICAMERA,		tmp );
	GET_FLT( m_mask_circular_upper_bound,	"DETECTION_UPPER_BOUND",	OMNICAMERA,		"0.0" );


	GET_FLT( m_foreground_pxl_update,		"FOREGROUND_PIXEL_UPDATE",	BKG_SUBTRACT,	"0.00300" );
	GET_FLT( m_background_pxl_update,		"BACKGROUND_PIXEL_UPDATE",	BKG_SUBTRACT,	"0.05000" );
	GET_INT( m_bkg_init_frames,				"INITIALISATION_FRAMES",	BKG_SUBTRACT,	"32"	);
	GET_FLT( m_max_shadow_darkening,		"MAX_SHADOW_DARKENING",		BKG_SUBTRACT,	"0.7"	);


	CString t;
	GET_STR(  t,							"METHOD",					TRACKING,		"C"	);
	if ( t.GetAt( 0 ) == 'B' )
		m_tracking_method = BLOB_TRACKING_METHOD;
	else
		m_tracking_method = COLOUR_TRACKING_METHOD;


	GET_INT(  m_min_blob_area,				"MIN_AREA",					BLOB_TRACKING,	"50"	);
	GET_INT(  m_min_blob_radial_size,		"MIN_RADIAL_SIZE",			BLOB_TRACKING,	"-1"	);
	GET_INT(  m_min_blob_distance,			"MIN_SPATIAL_DISTANCE",		BLOB_TRACKING,	"25"	);
	GET_INT(  m_min_blob_radial_distance,	"MIN_RADIAL_DISTANCE",		BLOB_TRACKING,	"-1"	);
	GET_FLT(  m_max_blob_frame_movement,	"MAX_FRAME_MOVEMENT",		BLOB_TRACKING,	"0.25"	);
	GET_FLT(  m_obj_state_update_rate,		"OBJ_STATE_UPDATE_RATE",	BLOB_TRACKING,	"0.4"	);
	GET_BOOL( m_do_remove_ghosts,			"REMOVE_GHOSTS",			BLOB_TRACKING,	"0"		);


	GET_INT(  m_min_colour_area,			"MIN_AREA",					COLOUR_TRACKING,	"50"	);
	GET_INT(  m_min_colour_radial_size,		"MIN_RADIAL_SIZE",			COLOUR_TRACKING,	"-1"	);
	GET_INT(  m_min_colour_distance,		"MIN_SPATIAL_DISTANCE",		COLOUR_TRACKING,	"25"	);
	GET_INT(  m_min_colour_radial_distance, "MIN_RADIAL_DISTANCE",		COLOUR_TRACKING,	"-1"	);
	GET_INT(  m_max_colour_frame_movement,	"MAX_FRAME_MOVEMENT",		COLOUR_TRACKING,	"25"	);
	GET_INT(  m_nearby_colour_threshold,	"NEARBY_THRESHOLD",			COLOUR_TRACKING,	"35"	);
	GET_INT(  m_colour_update_rate,			"EM_UPDATE_RATE",			COLOUR_TRACKING,	"100"	);
	GET_INT(  m_min_colour_update_size,		"EM_MIN_SIZE_LIMIT",		COLOUR_TRACKING,	"50"	);
	GET_INT(  m_max_colour_search_win,		"MAX_SEARCH_WINDOW",		COLOUR_TRACKING,	"40"	);
	GET_BOOL( m_sudden_death_rule,			"SUDDEN_DEATH_RULE",		COLOUR_TRACKING,	"0"		);


	GET_INT(  m_camera_control_method,		"METHOD",					CAMERA_CONTROLLER,	"1"		);
	GET_INT(  m_auto_tracked_targets,		"TRACKED_TARGETS",			CAMERA_CONTROLLER,	"2"		);
	GET_FLT(  m_min_track_zoom,				"MIN_AUTO_ZOOM",			CAMERA_CONTROLLER,	"1"		);
	GET_FLT(  m_max_track_zoom,				"MAX_AUTO_ZOOM",			CAMERA_CONTROLLER,	"6"		);
	GET_INT(  m_perspective_view_size_x,	"VIEW_SIZE_X",				CAMERA_CONTROLLER,	"240"	);
	GET_INT(  m_perspective_view_size_y,	"VIEW_SIZE_Y",				CAMERA_CONTROLLER,	"150"	);
	GET_INT(  m_interpolation_type,			"INTERPOLATION_TYPE",		CAMERA_CONTROLLER,	"3"		);


	GET_BOOL( m_save_history,				"SAVE_HISTORY",				OUTPUT,				"1"		);

	

	//--------------------------------------------------------------
	// splitting the image source into its constituent parts
	int pos = m_image_source.Find( "#" );
	if ( pos == -1 )
	{
		AfxMessageBox(	"The image sequence string does not contain any # characters!" );
		AfxMessageBox(	m_image_source );
		AfxMessageBox(	"A sequence of # characters is needed to indicate the numeric\n"
						"part of the filename." );
		exit( 9 );
	}

	// change the '###..#' sequence into a printf() compatible numeric format string of
	// the type:  '%0N.0Nd' where N is the length of the '###..#' string.
	CString num_part = m_image_source.Mid( pos ).SpanIncluding( "#" );
	sprintf( tmp, "%s%%0%d.0%dd%s", m_image_source.Mid( 0, pos ),
									num_part.GetLength(), 
									num_part.GetLength(),
									m_image_source.Mid( pos + num_part.GetLength() ) );
	m_image_source = tmp;
}




bool CfgParameters::readWindowPos( CString name, int& x, int& y )
{
	const char* GUI	= "__GUI__";

	const int TMP_SIZE = 500;
	char tmp[ TMP_SIZE ];

	name.Replace( ' ', '_' );

	GET_INT( x,	"$" + name + "_X",	GUI, "-1" );
	GET_INT( y,	"$" + name + "_Y",	GUI, "-1" );

	return x != -1 && y != -1;
}




void CfgParameters::saveWindowPos( CString name, int x, int y )
{
	const char* GUI	= "__GUI__";

	const int TMP_SIZE = 500;
	char tmp[ TMP_SIZE ];

	name.Replace( ' ', '_' );

	WRITE_INT( x,	"$" + name + "_X",	GUI );
	WRITE_INT( y,	"$" + name + "_Y",	GUI );
}




void CfgParameters::readWindowState( CString name, bool& visible )
{
	const char* GUI	= "__GUI__";

	const int TMP_SIZE = 500;
	char tmp[ TMP_SIZE ];

	name.Replace( ' ', '_' );

	visible = false;
	GET_BOOL( visible,	"$" + name + "_S",	GUI, "0" );
}




void CfgParameters::saveWindowState( CString name, bool visible )
{
	const char* GUI	= "__GUI__";

	name.Replace( ' ', '_' );

	WRITE_BOOL( visible, "$" + name + "_S",	GUI );
}

	


CfgParameters::~CfgParameters()
{
	if ( mask != 0 )
		cvReleaseImage( &mask );
	if ( azimuth_map != 0 )
		cvReleaseImage( &azimuth_map );
	if ( altitude_map != 0 )
		cvReleaseImage( &altitude_map );
	if ( radial_map != 0 )
		cvReleaseImage( &radial_map );

	delete[] p_azimuth_map;
	delete[] p_altitude_map;
	delete[] p_radial_map;

}




double CfgParameters::getMirrorFocalLen()
{
	double theta = ( 180.0 - m_mirror_vfov ) * DEG_2_RAD;
	return ( m_boundary_radius * ( 1.0 - cos( theta ) ) / sin( theta ) ) / 2.0;
}




double CfgParameters::getMirrorLowerBound()
{
	double H = getMirrorFocalLen() * 2.0;
	return H * sin( m_mask_circular_lower_bound * DEG_2_RAD ) 
		   / ( 1.0 + cos( m_mask_circular_lower_bound * DEG_2_RAD ) );
}



double CfgParameters::getMirrorUpperBound()
{
	double H = getMirrorFocalLen() * 2.0;
	return H * sin( m_mask_circular_upper_bound * DEG_2_RAD ) 
		   / ( 1.0 + cos( m_mask_circular_upper_bound * DEG_2_RAD ) );
}



void CfgParameters::constructMask()
{
	int size_x = ( getRoiMaxX() - getRoiMinX() +1 ) / getAspectRatioX();
	int size_y = ( getRoiMaxY() - getRoiMinY() +1 ) / getAspectRatioY();


	if ( mask != 0 )
		cvReleaseImage( &mask );
	mask = 0;

	mask = cvCreateImage( cvSize( size_x, size_y ), IPL_DEPTH_8U, 1 );
	assert( mask );

	iplSet( mask, RGB(0,0,0) );


	double H = getMirrorFocalLen() * 2.0;
	double l = H * sin( m_mask_circular_lower_bound * DEG_2_RAD ) 
			   / ( 1.0 + cos( m_mask_circular_lower_bound * DEG_2_RAD ) );
	double u = H * sin( m_mask_circular_upper_bound * DEG_2_RAD ) 
			   / ( 1.0 + cos( m_mask_circular_upper_bound * DEG_2_RAD ) );
	assert( l > u );


	cvCircle( mask, 
			  cvPoint( getMirrorCentreX(), getMirrorCentreY() ), 
			  u,
			  CV_RGB(255,255,255), 1 );

	cvCircle( mask, 
			  cvPoint( getMirrorCentreX(), getMirrorCentreY() ), 
			  l,
			  CV_RGB(255,255,255), 1 );

	cvFloodFill( mask, 
				 cvPoint( getMirrorCentreX() + u + 1, getMirrorCentreY() ), 
				 CV_RGB(255,255,255) );


	// other LUTs
	if ( azimuth_map != 0 )
		cvReleaseImage( &azimuth_map );
	azimuth_map = 0;

	if ( altitude_map != 0 )
		cvReleaseImage( &altitude_map );
	altitude_map = 0;

	if ( radial_map != 0 )
		cvReleaseImage( &radial_map );
	radial_map = 0;


	azimuth_map = cvCreateImage( cvSize( size_x, size_y ), IPL_DEPTH_32F, 1 );
	assert( azimuth_map );

	altitude_map = cvCreateImage( cvSize( size_x, size_y ), IPL_DEPTH_32F, 1 );
	assert( altitude_map );
	
	radial_map = cvCreateImage( cvSize( size_x, size_y ), IPL_DEPTH_32F, 1 );
	assert( radial_map );


	iplSetFP( azimuth_map, 0.0 );
	iplSetFP( altitude_map, 0.0 );
	iplSetFP( radial_map, 0.0 );


	int mirror_cx = getMirrorCentreX();
	int mirror_cy = getMirrorCentreY();
	double mirror_H = CfgParameters::instance()->getMirrorFocalLen() * 2.0;

	for ( int y = 0; y < size_y; ++y )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_azimuth, azimuth_map, y );
		COMPUTE_IMAGE_ROW_PTR( float, p_altitude, altitude_map, y );
		COMPUTE_IMAGE_ROW_PTR( float, p_radial, radial_map, y );

		for ( int x = 0; x < size_x; ++x )
		{
			double az = atan2( (double) y - mirror_cy, x - mirror_cx ) * RAD_2_DEG;
			if ( az < 0 )
				az += 360.0;

			p_azimuth[ x ] = az;
			
			
			double r = sqrt( (double) SQR( x - mirror_cx ) + SQR( y - mirror_cy ) );
			p_radial[ x ] = r;

			
			double mz = ( SQR( mirror_H ) - SQR( r ) ) / ( 2.0 * mirror_H );
			double rho = sqrt( SQR( r ) + SQR( mz ) );
			p_altitude[ x ] = asin( mz / rho ) * RAD_2_DEG;
		}
	}


	p_azimuth_map = new float* [ size_y ];
	for ( int y = 0; y < size_y; ++y )
		p_azimuth_map[ y ] = (float*) ( azimuth_map->imageData + y * azimuth_map->widthStep );

	p_altitude_map = new float* [ size_y ];
	for ( int y = 0; y < size_y; ++y )
		p_altitude_map[ y ] = (float*) ( altitude_map->imageData + y * altitude_map->widthStep );

	p_radial_map = new float* [ size_y ];
	for ( int y = 0; y < size_y; ++y )
		p_radial_map[ y ] = (float*) ( radial_map->imageData + y * radial_map->widthStep );
}




double CfgParameters::calcAzimuth( double x, double y )
{
	double az = atan2( y - m_mirror_centre_y, x - m_mirror_centre_x ) * RAD_2_DEG;
	if ( az < 0 )
		az += 360.0;

	return az;
}
