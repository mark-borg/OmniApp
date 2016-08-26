
#include "VirtualCameraController.h"
#include "..\gui\stdafx.h"
#include "CfgParameters.h"
#include "Pipeline.h"



const float VirtualCamera::zoom_accel_step	= 0.1f;
const float VirtualCamera::pan_accel_step	= 1.0f;
const float VirtualCamera::tilt_accel_step	= 0.5f;




void trackObject( ObjectList* objects, CameraList* cameras, int obj_id, int cam_id )
{
	CfgParameters* cfg = CfgParameters::instance();
	assert( cfg );

	VirtualCamera* cam = cameras->getAt( cam_id );

	if ( objects->getAt( obj_id ) )
	{
		Pipeline::PerspectiveDewarper_Type* dewarper = Pipeline::instance()->getPerspectiveNode( cam_id );
		if ( dewarper )
		{ 
			Object* obj = objects->getAt( obj_id );

			bool first_time = cam == 0;
			if ( first_time )
			{
				VirtualCamera new_cam;
				new_cam.obj_id = obj_id;

				cameras->add( cam_id, new_cam );
				
				cam = cameras->getAt( cam_id );
				assert( cam != 0 );

				dewarper->autoTracking( obj_id );
			}

			
			if ( dewarper->isManuallyLocked() )
			{
				// window has been locked by the user 
				// unlock the view from continuing tracking of this object
				Pipeline::PerspectiveDewarper_Type* dewarper = Pipeline::instance()->getPerspectiveNode( cam_id );
				if ( dewarper )
					dewarper->autoTracking();

				cameras->remove( cam_id );
				return;
			}


			float** p_azimuth_map = cfg->getAzimuthMapPtrs();
			float** p_altitude_map = cfg->getAltitudeMapPtrs();
			int width = cfg->getAzimuthMap()->width;
			int height = cfg->getAzimuthMap()->height;

			int cx = cfg->getMirrorCentreX();
			int cy = cfg->getMirrorCentreY();
			
			float old_vfov = dewarper->getVFOV();
			float old_hfov = dewarper->getHFOV();
			float old_zoom = dewarper->getZoom();

			
			// determine centre (az,el) of window
			int x = obj->centre.x;
			int y = obj->centre.y;

			float obj_az = p_azimuth_map[ y ][ x ] + 180;
			float obj_el = p_altitude_map[ y ][ x ] - old_vfov /2;

			if ( obj_az > 360 )
				obj_az -= 360;


			// determine bounds of object
			int tmx = cy + lrintf( obj->max_radius );
			int tmn = cy + lrintf( obj->min_radius );
			float obj_hfov = fabs( obj->max_azimuth - obj->min_azimuth );
			float obj_vfov = 0;
			if ( tmx < height && tmn < height )
				obj_vfov = fabs( p_altitude_map[ tmx ][ cx ] 
								 - p_altitude_map[ tmn ][ cx ] );
			else
				obj_vfov = fabs( cfg->calcAzimuth( cx, tmx ) 
								 - cfg->calcAzimuth( cx, tmn ) );

			// determine if any change in zoom is required
			float zoom = old_zoom;
			bool zooming_out = false;
			bool zooming_in = false;
			if ( 1.5 * obj_hfov > old_hfov || 1.5 * obj_vfov > old_vfov )
			{
				zoom -= cam->zoom_out_speed;
				zooming_out = true;
			}
			else if ( 2.5 * obj_hfov < old_hfov && 2.5 * obj_vfov < old_vfov )
			{
				zoom += cam->zoom_in_speed;
				zooming_in = true;
			}

			if ( zooming_out )
				cam->zoom_out_speed += VirtualCamera::zoom_accel_step;
			else
				cam->zoom_out_speed -= VirtualCamera::zoom_accel_step;

			if ( zooming_in )
				cam->zoom_in_speed += VirtualCamera::zoom_accel_step;
			else
				cam->zoom_in_speed -= VirtualCamera::zoom_accel_step;


			cam->zoom_out_speed = cam->zoom_out_speed < VirtualCamera::zoom_accel_step ? VirtualCamera::zoom_accel_step : cam->zoom_out_speed;
			cam->zoom_in_speed = cam->zoom_in_speed < VirtualCamera::zoom_accel_step ? VirtualCamera::zoom_accel_step : cam->zoom_in_speed;
			cam->zoom_out_speed = cam->zoom_out_speed > 1 ? 1 : cam->zoom_out_speed;
			cam->zoom_in_speed = cam->zoom_in_speed > 1 ? 1 : cam->zoom_in_speed;

			zoom = zoom > cfg->getTrackingMaxZoom() 
						? cfg->getTrackingMaxZoom() 
						: ( zoom < cfg->getTrackingMinZoom() 
								? cfg->getTrackingMinZoom() : zoom );


			//-------
			// determine if any change in panning is needed
			float old_az = dewarper->getPan();

			if ( old_az - obj_az > 180 )		// compensate for 0-360 boundary condition
				old_az -= 360;
			if ( obj_az - old_az > 180 )
				obj_az -= 360;

			float az = old_az;
			bool pan_inc = false;
			bool pan_dec = false;

			if ( obj_az > az )
			{
				cam->pan_inc_speed += VirtualCamera::pan_accel_step;
				cam->pan_dec_speed -= VirtualCamera::pan_accel_step * 2;
				pan_inc = true;
			}
			else if ( obj_az < az )
			{
				cam->pan_inc_speed -= VirtualCamera::pan_accel_step * 2;
				cam->pan_dec_speed += VirtualCamera::pan_accel_step;
				pan_dec = true;
			}

			cam->pan_inc_speed = cam->pan_inc_speed < 0 ? 0 : cam->pan_inc_speed;
			cam->pan_dec_speed = cam->pan_dec_speed < 0 ? 0 : cam->pan_dec_speed;
			cam->pan_inc_speed = cam->pan_inc_speed > 10 ? 10 : cam->pan_inc_speed;
			cam->pan_dec_speed = cam->pan_dec_speed > 10 ? 10 : cam->pan_dec_speed;


			float pan_net = cam->pan_inc_speed - cam->pan_dec_speed;

			float delta = fabs( obj_az - old_az );

			if ( pan_net < 0 )
			{
				float z = MIN( fabs( pan_net ), delta );
				if ( z > cam->pan_accel_step )
					az -= z;
			}
			else
			{
				float z = MIN( fabs( pan_net ), delta );
				if ( z > cam->pan_accel_step )
					az += z;
			}


			//-------
			// determine if any change in camera tilt is needed
			float old_el = dewarper->getTilt();

			float el = old_el;
			bool tilt_inc = false;
			bool tilt_dec = false;

			if ( obj_el > el )
			{
				cam->tilt_inc_speed += VirtualCamera::tilt_accel_step;
				cam->tilt_dec_speed -= VirtualCamera::tilt_accel_step * 2;
				tilt_inc = true;
			}
			else if ( obj_el < el )
			{
				cam->tilt_inc_speed -= VirtualCamera::tilt_accel_step * 2;
				cam->tilt_dec_speed += VirtualCamera::tilt_accel_step;
				tilt_dec = true;
			}

			cam->tilt_inc_speed = cam->tilt_inc_speed < 0 ? 0 : cam->tilt_inc_speed;
			cam->tilt_dec_speed = cam->tilt_dec_speed < 0 ? 0 : cam->tilt_dec_speed;
			cam->tilt_inc_speed = cam->tilt_inc_speed > 3 ? 3 : cam->tilt_inc_speed;
			cam->tilt_dec_speed = cam->tilt_dec_speed > 3 ? 3 : cam->tilt_dec_speed;


			float tilt_net = cam->tilt_inc_speed - cam->tilt_dec_speed;

			delta = fabs( obj_el - old_el );

			if ( tilt_net < 0 )
			{
				float e = MIN( fabs( tilt_net ), delta );
				if ( e > cam->tilt_accel_step )
					el -= e;
			}
			else
			{
				float e = MIN( fabs( tilt_net ), delta );
				if ( e > cam->tilt_accel_step )
					el += e;
			}



			// for the first time, centre window directly on the object
			if ( first_time )
			{
				az = obj_az;
				el = obj_el;
			}



			// any changes to viewing directions of camera?
			bool view_changed = false;

			if ( fabs( old_az - az ) > 1 || first_time )
			{
				dewarper->adjustPan( az );
				view_changed = true;
			}

			if ( fabs( old_el - el ) > 1 || first_time )
			{
				dewarper->adjustTilt( el );
				view_changed = true;
			}

			if ( zoom != old_zoom )
			{
				dewarper->adjustZoom( zoom );
				view_changed = true;
			}

			if ( first_time )
				dewarper->adjustRoll( 0 );

			if ( view_changed )
			{
				dewarper->updateMapping();
			}


			// has object gone out of view of the camera? (motion too fast)
			if ( ! first_time )
			{
				float half_vfov = dewarper->getVFOV() / 2;
				float half_hfov = dewarper->getHFOV() / 2;

				az = dewarper->getPan();
				el = dewarper->getTilt();

				if ( obj_az < ( az - half_hfov ) ||
					 obj_az > ( az + half_hfov ) ||
					 obj_el < ( el - half_vfov ) ||
					 obj_el > ( el + half_vfov ) )
				{
					dewarper->adjustPan( obj_az );
					dewarper->adjustTilt( obj_el );
					dewarper->updateMapping();						
				}

			}
		}
	}
	else if ( cam != 0 )	// this is the last time
	{
		// object died - unlock the view from continuing tracking of this object
		Pipeline::PerspectiveDewarper_Type* dewarper = Pipeline::instance()->getPerspectiveNode( cam_id );
		if ( dewarper )
			dewarper->autoTracking();

		cameras->remove( cam_id );
	}
}




void trackMethodManual1( ObjectList* objects, CameraList* cameras )
{
	//--- TRACKING MEHOD ---
	// objects to be tracked are selected manually by the user. The
	// camera to be used is selected in a round-bobbin fashion.
	//-----

	int max_obj_id = -1;
	int cam_free = -1;
	int untracked_obj = -1;


	// continue tracking existing (manually selected) objects
	for ( int c = 0; c < cameras->capacity(); ++c )
	{
		if ( ! cameras->isFree( c ) )
		{
			VirtualCamera* cam = cameras->getAt( c );
			trackObject( objects, cameras, cam->obj_id, c );

			max_obj_id = MAX( max_obj_id, cam->obj_id );
		}
	}
}




void trackMethodManual1Set( ObjectList* objects, CameraList* cameras, int obj_id )
{
	static int last_camera_used = -1;

	Pipeline::PerspectiveDewarper_Type* dewarper = 0;
	int bailout = cameras->capacity();

	do
	{
		++last_camera_used;
		if ( last_camera_used >= Pipeline::instance()->getNumPerspectives() )
			last_camera_used = 0;
		dewarper = Pipeline::instance()->getPerspectiveNode( last_camera_used );

		if ( bailout-- < 0 )
			return;			// all windows locked!
	}
	while( dewarper->isManuallyLocked() );


	if ( ! cameras->isFree( last_camera_used ) )
	{
		dewarper = Pipeline::instance()->getPerspectiveNode( last_camera_used );
		if ( dewarper )
			dewarper->autoTracking();

		cameras->remove( last_camera_used );
	}

	trackObject( objects, cameras, obj_id, last_camera_used );
}




void trackMethodAuto1( ObjectList* objects, CameraList* cameras )
{
	//--- TRACKING MEHOD ---
	// whenever a camera becomes free (is no longer tracking an object),
	// then the first untracked object on the queue is allocated to this
	// camera.
	//-----

	int max_obj_id = -1;
	int cam_free = -1;
	int untracked_obj = -1;


	// continue tracking existing objects
	for ( int c = 0; c < cameras->capacity(); ++c )
	{
		if ( ! cameras->isFree( c ) )
		{
			VirtualCamera* cam = cameras->getAt( c );
			trackObject( objects, cameras, cam->obj_id, c );

			max_obj_id = MAX( max_obj_id, cam->obj_id );
		}
		else if ( cam_free == -1 )		// find first free camera
		{				
			Pipeline::PerspectiveDewarper_Type* dewarper = Pipeline::instance()->getPerspectiveNode( c );
			if ( dewarper && ! dewarper->isManuallyLocked() )
				cam_free = c;
		}
	}


	// is there a new object which is not being tracked?
	for ( int o = max_obj_id + 1; o < objects->capacity(); ++o )
	{
		if ( ! objects->isFree( o ) )
		{
			untracked_obj = o;
			break;						// find first untracked object
		}
	}

	
	// is any of the cameras is free and is there an untracked object?
	if ( cam_free != -1 && untracked_obj != -1 )
	{
		trackObject( objects, cameras, untracked_obj, cam_free );
	}
}




