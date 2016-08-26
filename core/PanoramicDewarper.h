
#ifndef PANORAMIC_DEWARPER_H
#define PANORAMIC_DEWARPER_H


#pragma warning( disable : 4786 )

#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"



//#define DUMP_OUTPUT			




template< class ALLOCATOR >
class PanoramicDewarper
	: public ProcessorNode< IplData, IplData >
{

	ALLOCATOR* allocator;


	// geometrical maps for re-projection
	IplImage* m_geom_map_x;
	IplImage* m_geom_map_y;

	// some 1D look-up tables
	double* lut_xp;
	double* lut_yp;
	double* lut_distance;


	IplData dewarped;

	IplImageResource map_typ;
	IplImageResource img_typ;

	
	double	R[9], Rpan[9], Rroll[9];			// rotation matrices


	CCriticalSection safe_state;	// safe to change virtual camera parameters & geometric maps


	struct VirtualCamera
	{
		int		view_x, view_y;
		int		actual_view_x;			// actual width (minimum of circumference of cylinder or user-given window width view_x)
		double	principal_height;		// height of principal points on cylindrical surface
		double	zoom;
		double	pan;
		double	roll;
		double	tilt;
		bool	left_right;
		double	camera_focal_len;	


		VirtualCamera()
		{
			view_x = view_y = actual_view_x = 0;
			zoom = pan = roll = tilt = camera_focal_len = -1.0;
			left_right = true;
			principal_height = -999.0;
		}
	
	};

	VirtualCamera s;		// current virtual camera state
	VirtualCamera sn;		// new virtual camera state


	// mirror parameters
	bool	m_inverted_camera;
	int		m_mirror_centre_x, m_mirror_centre_y;
	float	m_mirror_vfov;
	int		m_mirror_focal_len;


	int		interpolation;


public:


	PanoramicDewarper( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		m_geom_map_x = 0;
		m_geom_map_y = 0;

		lut_xp = 0;
		lut_yp = 0;
		lut_distance = 0;

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



	~PanoramicDewarper()
	{
		if ( m_geom_map_x != 0 )
			allocator->release( m_geom_map_x );
		if ( m_geom_map_y != 0 )
			allocator->release( m_geom_map_y );
		if ( dewarped.img != 0 )
			allocator->release( dewarped.img );

		if ( lut_xp != 0 )
			delete[] lut_xp;
		if ( lut_yp != 0 )
			delete[] lut_yp;
		if ( lut_distance != 0 )
			delete[] lut_distance;
	}



	void adjustPanDelta( double pan_delta )
	{
		sn.pan += pan_delta * DEG_2_RAD;
	}



	void adjustRollDelta( double roll_delta )
	{
		sn.roll += roll_delta * DEG_2_RAD;
	}



	void adjustTiltDelta( double tilt_delta )
	{
		sn.tilt += tilt_delta * DEG_2_RAD;
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
		return sn.actual_view_x;
	}



	int height()
	{
		return sn.view_y;
	}



	double getHFOV()
	{
		assert( sn.camera_focal_len > 0 );
		
		return sn.actual_view_x / ( 2 * PI * sn.camera_focal_len ) * 360;
	}



	double getVFOV()
	{
		assert( sn.camera_focal_len > 0 );

		double len_t = sn.principal_height + sn.view_y;
		double len_b = sn.principal_height;

		double angle_t = atan( len_t / sn.camera_focal_len ) * RAD_2_DEG;
		double angle_b = atan( len_b / sn.camera_focal_len ) * RAD_2_DEG;

		return angle_t - angle_b;
	}



	void initMappingFov( double hfov, double vfov, 
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
		initMapping(	lrintf( 2 * PI * mirror_focal_len * hfov / 360.0 ),
						lrintf( tan( vfov * DEG_2_RAD ) * mirror_focal_len ),
						mirror_centre_x, mirror_centre_y, 
						mirror_focal_len,
						inverted_camera,
						mirror_vfov,
						left_right,
						zoom,
						pan,
						roll,
						tilt );
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
		if ( roll > 90 )	
			roll = 90;

		if ( roll < -90 )	
			roll = -90;

		if ( tilt > 70 )
			tilt = 70;

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
			s.actual_view_x = MIN( 2 * PI * s.camera_focal_len +1, s.view_x );


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

			if ( different_pan || different_roll )
			{
				MULT_3x3_MATRIX( Rpan, Rroll, R );
			}


			// has tilt changed?
			tilt = tilt * DEG_2_RAD;
			bool different_tilt = ( s.tilt != tilt );
			s.tilt = tilt;
		
			s.principal_height = s.camera_focal_len * tan( s.tilt );


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


			// create and generate look-up tables
			if ( different_image_size || different_zoom )
			{
				if ( lut_xp != 0 )
					delete[] lut_xp;
				if ( lut_yp != 0 )
					delete[] lut_yp;

				lut_xp = new double[ s.actual_view_x ];
				assert( lut_xp );
				lut_yp = new double[ s.actual_view_x ];
				assert( lut_yp );

				// for a point on the circumference of the cylinder,
				// we use this LUT to get the x,y coords of that point.
				// this LUT will change if radius (camera focus/zoom) or image
				// size change.
				for ( int vx = 0; vx < s.actual_view_x; ++vx )
				{
					double phi = vx / s.camera_focal_len;
					lut_xp[ vx ] = cos( phi ) * s.camera_focal_len;
					lut_yp[ vx ] = tan( phi ) * lut_xp[ vx ];
				}

				map_typ.clear( m_geom_map_x );
				map_typ.clear( m_geom_map_y );
			}


			// create and generate look-up table
			if ( different_image_size || different_zoom || different_tilt )
			{
				if ( lut_distance != 0 )
					delete[] lut_distance;

				lut_distance = new double[ s.view_y ];
				assert( lut_distance );

				// the distance factor in the denominator of the re-projection
				// equation only depends on height of pixel along cylinder's
				// surface. this LUT will change if radius (camera focus/zoom),
				// image size or tilt angle change.
				for ( int vy = 0; vy < s.view_y; ++vy )
				{
					double zp = s.view_y - vy + s.principal_height;
					lut_distance[ vy ] = sqrt( s.camera_focal_len * s.camera_focal_len + zp * zp );
				}
			}


			for ( int vy = 0; vy < s.view_y; ++vy )
			{
				double zp = s.view_y - vy + s.principal_height;
				double distance_factor = lut_distance[ vy ];

				int vvy = m_inverted_camera ? (s.view_y - vy -1) : vy;

				COMPUTE_IMAGE_ROW_PTR( float, map_x, m_geom_map_x, vvy );
				COMPUTE_IMAGE_ROW_PTR( float, map_y, m_geom_map_y, vvy );

				for ( int vx = 0; vx < s.actual_view_x; ++vx )
				{
					double xp = lut_xp[ vx ];
					double yp = lut_yp[ vx ];

					double rxp = R[0] * xp + R[1] * yp + R[2] * zp;
					double ryp = R[3] * xp + R[4] * yp + R[5] * zp;
					double rzp = R[6] * xp + R[7] * yp + R[8] * zp;
					
					double xi = ( m_mirror_focal_len*2 / ( rzp + distance_factor ) ) * rxp;
					double yi = ( m_mirror_focal_len*2 / ( rzp + distance_factor ) ) * ryp;

					double oxi = xi + m_mirror_centre_x;
					double oyi = yi + m_mirror_centre_y;


					int vvx = s.left_right ? vx : (s.view_x - vx -1);

					map_x[ vvx ] = oxi;
					map_y[ vvx ] = oyi;
				}
			}
		}
		safe_state.Unlock();

		// prepare new camera state by resetting it to current state.
		sn = s;
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
		safe_state.Lock();
		{
			dewarped.frame = buffer->frame;
			iplSet( dewarped.img, 0 );
			iplRemap( buffer->img, m_geom_map_x, m_geom_map_y, dewarped.img, interpolation );
		}
		safe_state.Unlock();

		// move result to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = dewarped;


#ifdef DUMP_OUTPUT			
		{
			char tmp[200];
			sprintf( tmp, "\\out\\%04.04d_panorama.bmp", dewarped.frame );
			cvSaveImage( tmp, dewarped.img );
		}
#endif
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



#endif
