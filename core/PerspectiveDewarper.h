
#ifndef PERSPECTIVE_DEWARPER_H
#define PERSPECTIVE_DEWARPER_H


#pragma warning( disable : 4786 )

#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"
#include "..\tools\DistinctColours.h"

 


template< class ALLOCATOR >
class PerspectiveDewarper
	: public ProcessorNode< IplData, IplData >
{
	
	ALLOCATOR* allocator;

	
	// geometrical maps for re-projection
	IplImage* m_geom_map_x;
	IplImage* m_geom_map_y;

	// look-up table
	IplImage* m_distance_map;


	IplData dewarped;

	IplImageResource map_typ;
	IplImageResource img_typ;


	double	R[9], Rpan[9], Rroll[9], Rtilt[9];		// rotation matrices


	CCriticalSection safe_state;	// safe to change virtual camera parameters & geometric maps


	struct VirtualCamera
	{
		int		view_x, view_y;
		double	zoom;
		double	pan;
		double	roll;
		double	tilt;
		bool	left_right;
		double	camera_focal_len;	

		VirtualCamera()
		{
			view_x = view_y = 0;
			zoom = pan = roll = tilt = camera_focal_len = -1.0;
			left_right = true;
		}
	
	};

	VirtualCamera s;		// current virtual camera state
	VirtualCamera sn;		// new virtual camera state


	// mirror parameters
	bool	m_inverted_camera;
	int		m_mirror_centre_x, m_mirror_centre_y;
	float	m_mirror_vfov;
	int		m_mirror_focal_len;


	double	hfov;
	double	vfov;

	
	static DistinctColours colours;
	CvScalar colour_code;
	int		auto_tracking_target;
	bool	manual_lock;

	int		interpolation;
	

public:


	PerspectiveDewarper( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		m_geom_map_x = 0;
		m_geom_map_y = 0;
		m_distance_map = 0;

		hfov = -1.0;
		vfov = -1.0;

		auto_tracking_target = -1;
		colour_code = colours.next();
		manual_lock = false;

		int i = CfgParameters::instance()->getInterpolationType();
		switch( i )
		{
		case 1:		interpolation = IPL_INTER_NN;
					break;

		case 2:		interpolation = IPL_INTER_LINEAR;
					break;
		case 3:	
		default:	interpolation = IPL_INTER_CUBIC;
					break;
		}
	}



	~PerspectiveDewarper()
	{
		if ( m_geom_map_x != 0 )
			allocator->release( m_geom_map_x );
		if ( m_geom_map_y != 0 )
			allocator->release( m_geom_map_y );
		if ( dewarped.img != 0 )
			allocator->release( dewarped.img );
		if ( m_distance_map != 0 )
			allocator->release( m_distance_map );
	}



	void adjustPanDelta( double pan_delta )
	{
		sn.pan += pan_delta * DEG_2_RAD;
	}



	void adjustPan( double new_pan )
	{
		sn.pan = new_pan * DEG_2_RAD;
	}



	void adjustRoll( double new_roll )
	{
		sn.roll = new_roll * DEG_2_RAD;
	}



	void adjustRollDelta( double roll_delta )
	{
		sn.roll += roll_delta * DEG_2_RAD;
	}



	void adjustTiltDelta( double tilt_delta )
	{
		sn.tilt += tilt_delta * DEG_2_RAD;
	}



	void adjustTilt( double new_tilt )
	{
		sn.tilt = new_tilt * DEG_2_RAD;
	}



	double getZoom()
	{
		return sn.zoom;
	}



	void adjustZoom( double new_zoom )
	{
		sn.zoom = new_zoom;
	}



	double getPan()
	{
		return sn.pan * RAD_2_DEG;
	}



	double getRoll()
	{
		return sn.roll * RAD_2_DEG;
	}



	double getTilt()
	{
		return sn.tilt * RAD_2_DEG;
	}



	bool isInvertedCamera()
	{
		return m_inverted_camera;
	}



	bool isLeftRight()
	{
		return sn.left_right;
	}



	void toggleLeftRight()
	{
		sn.left_right = ! sn.left_right;
	}



	int width()
	{
		return sn.view_x;
	}



	int height()
	{
		return sn.view_y;
	}



	void autoTracking( int obj_id = -1 )
	{
		auto_tracking_target = obj_id;
	}



	bool isAutoTracking()
	{
		return auto_tracking_target != -1;
	}



	void toggleManualLock()
	{
		manual_lock = ! manual_lock;
	}



	bool isManuallyLocked()
	{
		return manual_lock;
	}



	double getHFOV()
	{
		if ( hfov < 0 )
			calcHFOV();

		return hfov;
	}



	void calcHFOV()
	{
		float** p_azimuth_map = CfgParameters::instance()->getAzimuthMapPtrs();

		COMPUTE_IMAGE_ROW_PTR( float, map_row1_x, m_geom_map_x, 0 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row1_y, m_geom_map_y, 0 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row2_x, m_geom_map_x, height() -1 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row2_y, m_geom_map_y, height() -1 );

		int lut_width = CfgParameters::instance()->getAltitudeMap()->width;
		int lut_height = CfgParameters::instance()->getAltitudeMap()->height;

		int x0 = lrintf( map_row1_x[ 0 ] );				// top-left
		int y0 = lrintf( map_row1_y[ 0 ] );

		int x1 = lrintf( map_row1_x[ width()-1 ] );		// top-right
		int y1 = lrintf( map_row1_y[ width()-1 ] );

		int x2 = lrintf( map_row2_x[ width()-1 ] );		// bottom-right
		int y2 = lrintf( map_row2_y[ width()-1 ] );

		int x3 = lrintf( map_row2_x[ 0 ] );				// bottom-left
		int y3 = lrintf( map_row2_y[ 0 ] );

		hfov = 0.0;
		int count = 0;

		if ( y0 >= 0 && y0 < lut_height &&
			 y1 >= 0 && y1 < lut_height &&
			 x0 >= 0 && x0 < lut_width &&
			 x1 >= 0 && x1 < lut_width )
		{
			float az0 = p_azimuth_map[ y0 ][ x0 ];
			float az1 = p_azimuth_map[ y1 ][ x1 ];

			hfov += az1 - az0;
			++count;
		}

		if ( y2 >= 0 && y2 < lut_height &&
			 y3 >= 0 && y3 < lut_height &&
			 x2 >= 0 && x2 < lut_width &&
			 x3 >= 0 && x3 < lut_width )
		{
			float az2 = p_azimuth_map[ y2 ][ x2 ];
			float az3 = p_azimuth_map[ y3 ][ x3 ];

			hfov += az2 - az3;
			++count;
		}

		hfov = fabs( hfov / count );
	}



	double getVFOV()
	{
		if ( vfov < 0 )
			calcVFOV();

		return vfov;
	}



	void calcVFOV()
	{
		float** p_altitude_map = CfgParameters::instance()->getAltitudeMapPtrs();

		COMPUTE_IMAGE_ROW_PTR( float, map_row1_x, m_geom_map_x, 0 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row1_y, m_geom_map_y, 0 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row2_x, m_geom_map_x, height() -1 );
		COMPUTE_IMAGE_ROW_PTR( float, map_row2_y, m_geom_map_y, height() -1 );

		int lut_width = CfgParameters::instance()->getAltitudeMap()->width;
		int lut_height = CfgParameters::instance()->getAltitudeMap()->height;

		int x0 = lrintf( map_row1_x[ 0 ] );				// top-left
		int y0 = lrintf( map_row1_y[ 0 ] );

		int x1 = lrintf( map_row1_x[ width()-1 ] );		// top-right
		int y1 = lrintf( map_row1_y[ width()-1 ] );

		int x2 = lrintf( map_row2_x[ width()-1 ] );		// bottom-right
		int y2 = lrintf( map_row2_y[ width()-1 ] );

		int x3 = lrintf( map_row2_x[ 0 ] );				// bottom-left
		int y3 = lrintf( map_row2_y[ 0 ] );

		vfov = 0.0;
		int count = 0;

		if ( y0 >= 0 && y0 < lut_height &&
			 y3 >= 0 && y3 < lut_height &&
			 x0 >= 0 && x0 < lut_width &&
			 x3 >= 0 && x3 < lut_width )
		{
			float el0 = p_altitude_map[ y0 ][ x0 ];
			float el3 = p_altitude_map[ y3 ][ x3 ];

			vfov += el3 - el0;
			++count;
		}

		if ( y1 >= 0 && y1 < lut_height &&
			 y2 >= 0 && y2 < lut_height &&
			 x1 >= 0 && x1 < lut_width &&
			 x2 >= 0 && x2 < lut_width )
		{
			float el1 = p_altitude_map[ y1 ][ x1 ];
			float el2 = p_altitude_map[ y2 ][ x2 ];

			vfov += el2 - el1;
			++count;
		}
		
		vfov = fabs( vfov / count );
	}



	CvScalar getColourCode()
	{
		if ( isAutoTracking() )
			return colour_code;

		CvScalar dim_colour = colour_code;
		unsigned char* ptr = (unsigned char*) &dim_colour;

		ptr[0] /= 3;
		ptr[1] /= 3;
		ptr[2] /= 3;
		ptr[3] /= 3;

		ptr[0] *= 2;
		ptr[1] *= 2;
		ptr[2] *= 2;
		ptr[3] *= 2;

		return dim_colour;
	}



	double getWidth()
	{
		return sn.view_x;
	}


	
	const IplImage* getGeomMapX()
	{
		return m_geom_map_x;
	}



	const IplImage* getGeomMapY()
	{
		return m_geom_map_y;
	}



	void initMapping(	double hfov, double vfov, 
						int mirror_centre_x, int mirror_centre_y, 
						int parabolic_radius,
						bool inverted_camera,
						float mirror_vfov )
	{
		double focal_length = parabolic_radius / 2;

		initMapping(	lrintf( 2 * PI * focal_length * hfov / 360.0 ),
						lrintf( tan( vfov * DEG_2_RAD ) * focal_length ),
						mirror_centre_x, mirror_centre_y, 
						parabolic_radius,
						inverted_camera,
						mirror_vfov,
						true, 1, 0, 0, 0 );
	}



	void updateMapping()
	{
		initMapping( sn.view_x, sn.view_y,
					 m_mirror_centre_x, m_mirror_centre_y,
					 m_mirror_focal_len,
					 m_inverted_camera,
					 m_mirror_vfov * RAD_2_DEG,
					 sn.left_right,
					 sn.zoom,
					 sn.pan * RAD_2_DEG,
					 sn.roll * RAD_2_DEG,
					 sn.tilt * RAD_2_DEG );
	}



	void initMapping(	int view_x, int view_y, 
						int mirror_centre_x, int mirror_centre_y, 
						int mirror_focal_len,
						bool inverted_camera,
						float mirror_vfov,
						bool left_right,
						double zoom,
						double pan,
						double roll,
						double tilt )
	{
		// some range checking on given variables
		if ( roll > 30 )	
			roll = 30;

		if ( roll < -30 )	
			roll = -30;

		if ( tilt > 90 )
			tilt = 90;

		if ( tilt < ( 90 - mirror_vfov ) )
			tilt = ( 90 - mirror_vfov );	

		while ( pan > 360 )
			pan -= 360;

		while ( pan > 360 )
			pan -= 360;


		safe_state.Lock();
		{
			m_mirror_centre_x = mirror_centre_x;
			m_mirror_centre_y = mirror_centre_y;
			m_mirror_focal_len = mirror_focal_len;
			m_mirror_vfov = mirror_vfov * DEG_2_RAD;


			// is image size different than before?
			bool different_image_size = ( s.view_x != view_x || s.view_y != view_y );
			s.view_x = view_x;
			s.view_y = view_y;


			m_inverted_camera = inverted_camera;
			s.left_right = left_right;


			// has zoom changed?
			bool different_zoom = ( s.zoom != zoom );
			s.zoom = zoom;
			s.camera_focal_len = m_mirror_focal_len * zoom;


			// has pan changed?
			pan = pan * DEG_2_RAD;
			bool different_pan = ( s.pan != pan );
			s.pan = pan;

			if ( different_pan )
			{
				double tmp_pan = s.pan + PI;	// default pan = 180, to keep top part of omni-image in first part of panorama

				Rpan[0] = cos( tmp_pan );	Rpan[1] = -sin( tmp_pan );	Rpan[2] = 0;
				Rpan[3] = sin( tmp_pan );	Rpan[4] = cos( tmp_pan );	Rpan[5] = 0;
				Rpan[6] = 0;				Rpan[7] = 0;				Rpan[8] = 1;
			}


			// has roll changed?
			roll = roll * DEG_2_RAD;
			bool different_roll = ( s.roll != roll );
			s.roll = roll;

			if ( different_roll )
			{
				Rroll[0] = 1;	Rroll[1] = 0;				Rroll[2] = 0;
				Rroll[3] = 0;	Rroll[4] = cos( s.roll );	Rroll[5] = -sin( s.roll );
				Rroll[6] = 0;	Rroll[7] = sin( s.roll );	Rroll[8] = cos( s.roll );
			}


			// has tilt changed?
			tilt = tilt * DEG_2_RAD;
			bool different_tilt = ( s.tilt != tilt );
			s.tilt = tilt;
		
			if ( different_tilt )
			{
				Rtilt[0] = cos( -s.tilt );	Rtilt[1] = 0;	Rtilt[2] = sin( -s.tilt );
				Rtilt[3] = 0;				Rtilt[4] = 1;	Rtilt[5] = 0;
				Rtilt[6] = -sin( -s.tilt );	Rtilt[7] = 0;	Rtilt[8] = cos( -s.tilt );
			}

			if ( different_pan || different_roll || different_tilt )
			{
				double T[9];

				MULT_3x3_MATRIX( Rtilt, Rroll, T );
				MULT_3x3_MATRIX( Rpan, T, R );
			}



			// create geometric maps
			if ( different_image_size )
			{
				map_typ = IplImageResource( s.view_x, s.view_y, IPL_DEPTH_32F, 1 );
				img_typ = IplImageResource( s.view_x, s.view_y, IPL_DEPTH_8U, 3 );

				if ( m_geom_map_x != 0 )
					allocator->release( m_geom_map_x );
				if ( m_geom_map_y != 0 )
					allocator->release( m_geom_map_y );
				
				m_geom_map_x = allocator->acquire( map_typ );
				m_geom_map_y = allocator->acquire( map_typ );
			}
			assert( m_geom_map_x );
			assert( m_geom_map_y );


			// create LUT
			if ( different_image_size || different_zoom )
			{
				if ( m_distance_map != 0 )
					allocator->release( m_distance_map );
				
				m_distance_map = allocator->acquire( map_typ );

				// the distance factor in the denominator of the re-projection
				// equation only depends on the distance of the pixel in the
				// image plane with respect to the principal point and the focal
				// length of the camera. this LUT will change if camera focus/zoom
				// or image size change.
				for ( int vy = 0; vy < s.view_y; ++vy )
				{
					COMPUTE_IMAGE_ROW_PTR( float, map_d, m_distance_map, vy );

					for ( int vx = - s.view_x/2; vx < s.view_x/2; ++vx )
						map_d[ vx + s.view_x/2 ] = sqrt( s.camera_focal_len * s.camera_focal_len + vx * vx + vy * vy );
				}
			}
			assert( m_distance_map );



			double xp = s.camera_focal_len;
			for ( int vy =  0; vy < s.view_y; ++vy )
			{
				double zp = vy;

				int vvy = ( m_inverted_camera ? vy : s.view_y - vy -1 );

				COMPUTE_IMAGE_ROW_PTR( float, map_x, m_geom_map_x, vvy );
				COMPUTE_IMAGE_ROW_PTR( float, map_y, m_geom_map_y, vvy );
				COMPUTE_IMAGE_ROW_PTR( float, map_d, m_distance_map, vy );

				for ( int vx = - s.view_x/2; vx < s.view_x/2; ++vx )
				{
					double yp = vx;

					double rxp = R[0] * xp + R[1] * yp + R[2] * zp;
					double ryp = R[3] * xp + R[4] * yp + R[5] * zp;
					double rzp = R[6] * xp + R[7] * yp + R[8] * zp;
					
					double distance_factor = map_d[ vx + s.view_x/2 ];

					double xi = ( m_mirror_focal_len*2 / ( rzp + distance_factor ) ) * rxp;
					double yi = ( m_mirror_focal_len*2 / ( rzp + distance_factor ) ) * ryp;

					double oxi = xi + m_mirror_centre_x;
					double oyi = yi + m_mirror_centre_y;

					int vvx = ( s.left_right ? vx + (s.view_x/2) : (s.view_x - (vx + s.view_x/2) -1) );

					map_x[ vvx ] = oxi;
					map_y[ vvx ] = oyi;
				}
			}
		}
		safe_state.Unlock();

		// prepare new camera state by resetting it to current state.
		sn = s;


		// pre-calculate some stuff
		calcHFOV();
		calcVFOV();
	}



	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		// acquire result image buffer
		assert( dewarped.img == 0 );
		dewarped.img = allocator->acquire( img_typ );
		assert( dewarped.img );

		// get source
		IplData* buffer = & source_buffers[ 0 ];
		assert( buffer );
		assert( buffer->img );

		assert( m_geom_map_x );
		assert( m_geom_map_y );


		// dewarp
		dewarped.frame = buffer->frame;
		iplSet( dewarped.img, 0 );

		safe_state.Lock();
		{
			iplRemap( buffer->img, m_geom_map_x, m_geom_map_y, dewarped.img, interpolation );
		}
		safe_state.Unlock();


		// indicate auto-tracking mode, if any.
		if ( auto_tracking_target != -1 )
		{
			const int offset = 3;
			cvRectangle( dewarped.img, 
						 cvPoint( offset, offset ), 
						 cvPoint( dewarped.img->width - offset, dewarped.img->height - offset ), 
						 colour_code, 2 );
		}


		// move result to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = dewarped;
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( dewarped.img != 0 )
		{
			allocator->release( dewarped.img );
			dewarped.img = 0;
		}
	}

};



template< class ALLOCATOR >
DistinctColours PerspectiveDewarper< ALLOCATOR >::colours;



#endif
