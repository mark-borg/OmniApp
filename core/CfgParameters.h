
#ifndef CFG_PARAMETERS_H
#define CFG_PARAMETERS_H
 

#define WIN32_LEAN_AND_MEAN		// exclude rarely-used stuff from Windows headers
#include <afx.h>				// for CString
#include <ipl.h>
#include "..\tools\singleton.h"
#include "..\tools\CvUtils.h"




class CfgParameters : public Singleton< CfgParameters >
{
	CString	m_image_source;
	int		m_start_frame;
	int		m_end_frame;
	int		m_read_step;
	int		m_fps;

	bool	m_auto_calibrate;

	int		m_read_roi_min_x;
	int		m_read_roi_min_y;
	int		m_read_roi_max_x;
	int		m_read_roi_max_y;

	int		m_in_aspect_x;
	int		m_in_aspect_y;
	int		m_out_aspect_x;
	int		m_out_aspect_y;

	int		m_mirror_centre_x;
	int		m_mirror_centre_y;

	int		m_boundary_radius;
	float	m_mirror_vfov;

	bool	m_camera_inverted;

	IplImage* mask;

	float	m_mask_circular_lower_bound;
	float	m_mask_circular_upper_bound;


	double	m_foreground_pxl_update;
	double	m_background_pxl_update;

	int		m_bkg_init_frames;

	double	m_max_shadow_darkening;

	int		m_tracking_method;

	bool	m_do_remove_ghosts;
	int		m_min_blob_area;
	int		m_min_blob_radial_size;
	int		m_min_blob_distance;
	int		m_min_blob_radial_distance;
	float	m_max_blob_frame_movement;

	int		m_min_colour_area;
	int		m_min_colour_radial_size;
	int		m_min_colour_distance;
	int		m_min_colour_radial_distance;
	int		m_max_colour_frame_movement;
	int		m_colour_update_rate;
	int		m_min_colour_update_size;
	int		m_nearby_colour_threshold;
	int		m_max_colour_search_win;
	bool	m_sudden_death_rule;

	float	m_obj_state_update_rate;

	int		m_auto_tracked_targets;
	int		m_camera_control_method;
	float	m_max_track_zoom;	
	float	m_min_track_zoom;	
	int		m_perspective_view_size_x;
	int		m_perspective_view_size_y;
	int		m_interpolation_type;

	bool	m_save_history;


	static 	CString param_file_name;


	IplImage* radial_map;
	IplImage* azimuth_map;
	IplImage* altitude_map;

	float** p_radial_map;
	float** p_azimuth_map;
	float** p_altitude_map;


public:


	// camera control methods
	enum {	AUTO_CONTROL_METHOD_1		= 1,
			MANUAL_CONTROL_METHOD_1		= 2	  };

	// object tracking methods
	enum {	BLOB_TRACKING_METHOD		= 1,
			COLOUR_TRACKING_METHOD		= 2	  };



	static void setParamFilename( CString param_file_name );



	CString& getImageSource()
	{
		return m_image_source;
	}


	int& getStartFrame()
	{
		return m_start_frame;
	}


	int& getEndFrame()
	{
		return m_end_frame;
	}


	int& getReadStep()
	{
		return m_read_step;
	}


	int& getFps()
	{
		return m_fps;
	}


	int& getRoiMinX()
	{
		return m_read_roi_min_x;
	}


	int& getRoiMinY()
	{
		return m_read_roi_min_y;
	}


	int& getRoiMaxX()
	{
		return m_read_roi_max_x;
	}


	int& getRoiMaxY()
	{
		return m_read_roi_max_y;
	}


	int& getInAspectX()
	{
		return m_in_aspect_x;
	}


	int& getInAspectY()
	{
		return m_in_aspect_y;
	}


	int& getOutAspectX()
	{
		return m_out_aspect_x;
	}


	int& getOutAspectY()
	{
		return m_out_aspect_y;
	}

	
	int& getMirrorCentreX()
	{
		return m_mirror_centre_x;
	}

	
	int& getMirrorCentreY()
	{
		return m_mirror_centre_y;
	}


	int& getMirrorBoundary()
	{
		return m_boundary_radius;
	}


	float& getMirrorLowerBoundAngle()
	{
		return m_mask_circular_lower_bound;
	}


	float& getMirrorUpperBoundAngle()
	{
		return m_mask_circular_upper_bound;
	}


	double getMirrorLowerBound();


	double getMirrorUpperBound();


	float& getMirrorVfov()
	{
		return m_mirror_vfov;
	}


	bool& isCameraInverted()
	{
		return m_camera_inverted;
	}


	int getAutoTrackedTargets()
	{
		return m_auto_tracked_targets;
	}


	bool& doAutoCalibration()
	{
		return m_auto_calibrate;
	}



	IplImage* getMask()
	{
		return mask;
	}



	IplImage* getAzimuthMap()
	{
		return azimuth_map;
	}



	IplImage* getAltitudeMap()
	{
		return altitude_map;
	}



	IplImage* getRadialMap()
	{
		return radial_map;
	}



	float** getAzimuthMapPtrs()
	{
		return p_azimuth_map;
	}



	float** getAltitudeMapPtrs()
	{
		return p_altitude_map;
	}



	float** getRadialMapPtrs()
	{
		return p_radial_map;
	}


	double calcAzimuth( double x, double y );


	void constructMask();


	bool isUsingROI()
	{
		return	m_read_roi_min_x != -1 &&
				m_read_roi_min_y != -1 &&
				m_read_roi_max_x != -1 &&
				m_read_roi_max_y != -1;
	}



	bool needAspectCorrection()
	{
		return	m_in_aspect_x != 1 ||
				m_in_aspect_y != 1 ||
				m_out_aspect_x != 1 ||
				m_out_aspect_y != 1;
	}



	double getAspectRatioX()
	{
		return m_in_aspect_x / (double) m_out_aspect_x;
	}



	double getAspectRatioY()
	{
		return m_in_aspect_y / (double) m_out_aspect_y;
	}

	
	
	double getMirrorFocalLen();



	int getBackgroundInitFrames()
	{
		return m_bkg_init_frames;
	}



	double getForegroundPxlUpdate()
	{
		return m_foreground_pxl_update;
	}



	double getBackgroundPxlUpdate()
	{
		return m_background_pxl_update;
	}



	double getMaxShadowDarkening()
	{
		return m_max_shadow_darkening;
	}



	int getTrackingMethod()
	{
		return m_tracking_method;
	}



	bool getRemoveGhosts()
	{
		return m_do_remove_ghosts;
	}



	int getMinBlobArea()
	{
		return m_min_blob_area;
	}



	int getMinColourArea()
	{
		return m_min_colour_area;
	}



	int getMinBlobRadialSize()
	{
		return m_min_blob_radial_size;
	}



	int getMinColourRadialSize()
	{
		return m_min_colour_radial_size;
	}



	int getMinBlobSpatialDistance()
	{
		return m_min_blob_distance;
	}



	int getMinBlobRadialDistance()
	{
		return m_min_blob_radial_distance;
	}



	int getMinColourSpatialDistance()
	{
		return m_min_colour_distance;
	}



	int getMinColourRadialDistance()
	{
		return m_min_colour_radial_distance;
	}



	float getMaxBlobFrameMovement()
	{
		return m_max_blob_frame_movement;
	}



	int getMaxColourFrameMovement()
	{
		return m_max_colour_frame_movement;
	}



	int getNearbyColourThreshold()
	{
		return m_nearby_colour_threshold;
	}


	float getObjectStateUpdateRate()
	{
		return m_obj_state_update_rate;
	}



	int getColourUpdateRate()
	{
		return m_colour_update_rate;
	}



	int getMinColourUpdateSize()
	{
		return m_min_colour_update_size;
	}



	int getMaxColourSearchWindow()
	{
		return m_max_colour_search_win;
	}



	bool getSuddenDeathRule()
	{
		return m_sudden_death_rule;
	}



	int getCameraControlMethod()
	{
		return m_camera_control_method;
	}



	float getTrackingMaxZoom()
	{
		return m_max_track_zoom;
	}



	float getTrackingMinZoom()
	{
		return m_min_track_zoom;
	}



	int getPerspectiveViewSizeX()
	{
		return m_perspective_view_size_x;
	}



	int getPerspectiveViewSizeY()
	{
		return m_perspective_view_size_y;
	}



	int getInterpolationType()
	{
		return m_interpolation_type;
	}


	bool doSaveHistory()
	{
		return m_save_history;
	}
	

	bool readWindowPos( CString name, int& x, int& y );

	void saveWindowPos( CString name, int x, int y );
	
	void readWindowState( CString name, bool& visible );

	void saveWindowState( CString name, bool visible );



protected:

	// this class cannot be constructed directly because it is a 
	// Singleton class - the singleton must be obtained through
	// calling static member function Instance()

	CfgParameters();



	virtual ~CfgParameters();



	// since we made ctor and dtor protected, we must declare the
	// following template classes as friends.  Singleton<A> will need
	// to call the constructor to create the one and only instance;
	// while SingletonDestroyer<A> will need to call the destructor
	// when destroying the one and only instance.
	friend class Singleton< CfgParameters >;
	friend class SingletonDestroyer< CfgParameters >;

};



#endif
