
#ifndef VIRTUAL_CAMERA_CONTROLLER_H
#define VIRTUAL_CAMERA_CONTROLLER_H


#pragma warning( disable : 4786 )
 
#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "ObjectsInformation.h"
#include "..\gui\resource.h"




struct VirtualCamera
{
	int obj_id;

	float zoom_in_speed;
	float zoom_out_speed;

	float pan_inc_speed;
	float pan_dec_speed;

	float tilt_inc_speed;
	float tilt_dec_speed;

	static const float zoom_accel_step;
	static const float pan_accel_step;
	static const float tilt_accel_step;


	VirtualCamera()
	{
		obj_id = -1;

		zoom_in_speed = zoom_accel_step;
		zoom_out_speed = zoom_accel_step;

		pan_inc_speed = 0.0f;
		pan_dec_speed = 0.0f;

		tilt_inc_speed = 0.0f;
		tilt_dec_speed = 0.0f;
	}
};




typedef LWList< VirtualCamera >	CameraList;




void trackMethodAuto1( ObjectList* objects, CameraList* cameras );

void trackMethodManual1( ObjectList* objects, CameraList* cameras );

void trackMethodManual1Set( ObjectList* objects, CameraList* cameras, int obj_id );




template< class ALLOCATOR >
class VirtualCameraController
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;

	int	method;
	bool first_time;

	
	CameraList* cameras;

	
	// initial capacity for the list (same value is used for the re-grow parameter)
	enum{ CAMERAS_INIT_LIST_CAPACITY = 6 };


public:



	VirtualCameraController( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		method = CfgParameters::instance()->getCameraControlMethod();

		first_time = true;


		cameras = new CameraList( CAMERAS_INIT_LIST_CAPACITY, CAMERAS_INIT_LIST_CAPACITY );
		assert( cameras );
	}



	~VirtualCameraController()
	{
		delete cameras;
	}
	


	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		if ( method == CfgParameters::AUTO_CONTROL_METHOD_1 )
		{
			if ( first_time )
			{
				int windows_needed = MAX( CfgParameters::instance()->getAutoTrackedTargets(), 1 );
				int windows_now = Pipeline::instance()->getNumPerspectives();

				for ( int k = windows_now; k < windows_needed; ++k )
				{
					PostThreadMessage( AfxGetApp()->m_nThreadID, IDC_CREATE_VIEW3, 0, 0 );
					
					int bailout = 30;
					do
					{
						Sleep( 100 );	// window creation (done by GUI thread) may take some time
						--bailout;
					}
					while( Pipeline::instance()->getNumPerspectives() == windows_now && bailout );
					++windows_now;
				}

				Sleep( 100 );	// allow some time for GUI thread to catch-up
				PostThreadMessage( AfxGetApp()->m_nThreadID, ID_SHOW_PERSPECTIVE_WINDOWS, 0, 0 );

				first_time = false;
			}


			if ( objects->size() )
			{
				trackMethodAuto1( ::objects, cameras );
			}
		}
		else if ( method = CfgParameters::MANUAL_CONTROL_METHOD_1 )
		{
			if ( objects->size() )
			{
				trackMethodManual1( ::objects, cameras );
			}
		}
		else
		{
			if ( first_time )
			{
				CLOG( "Unknown tracking method specified for the Virtual Camera Controller!", 0x0000ff );
				first_time = false;
			}
		}


		// move input (unchanged) to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = source_buffers[ 0 ];
	}



	void selectObjectForTracking( int obj_id )
	{
		trackMethodManual1Set( ::objects, cameras, obj_id );
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );
	}

};



#endif
