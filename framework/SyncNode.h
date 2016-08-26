
#ifndef SYNC_NODE_H
#define SYNC_NODE_H


#include "ProcessorNode.h"
#include "..\tools\timer.h"





template< class T_SOURCE_DATA, class T_SINK_DATA >  
class SyncNode : public ProcessorNode< T_SOURCE_DATA, T_SINK_DATA >
{
private:

	HANDLE timer;
	Timer clock;
	long orig_sync_period;

	long sync_period;
	long allowance;
	long refresh_count;

	enum { REFRESH = 30 };



public:

	SyncNode( string my_name, long sync_period )
		:	ProcessorNode< T_SOURCE_DATA, T_SINK_DATA >( my_name )
	{
		// create timer object
		timer = CreateWaitableTimer( NULL,		// security attributes
									 FALSE,		// manual reset?
									 NULL );	// event name
		assert( timer );


		assert( sync_period > 0 );
		orig_sync_period = sync_period;
		allowance = -10;
		this->sync_period = orig_sync_period + allowance;

		refresh_count = REFRESH;
	}



	~SyncNode()
	{
		CloseHandle( timer );
	}



	void processData()
	{
		// sync on wait timer
		WaitForSingleObject( timer, INFINITE );

		if ( ! --refresh_count )
		{
			clock.stop();

			double actual = clock.duration() * 1000;
			double expected = orig_sync_period * REFRESH;

			allowance = ( actual - expected ) / REFRESH;
			sync_period = orig_sync_period + allowance;

			refresh_count = REFRESH;

			// re-activate timer
			LARGE_INTEGER start_time;
			start_time.QuadPart = -10000 * sync_period +10;	

			SetWaitableTimer(	timer,
								&start_time,
								sync_period,				// periodic timer ( millisecs )
								NULL,						// completion routine
								NULL,						// completion routine argument
								FALSE );					// do not resume from suspended power mode
			clock.start();
		}

		ProcessorNode< T_SOURCE_DATA, T_SINK_DATA >::processData();
	}



	virtual UINT loop()
	{
		LARGE_INTEGER start_time;		
		start_time.QuadPart = -10000;	// allow a 1 micro-second delay (in hundreds of nanoseconds)

		SetWaitableTimer(	timer,
							&start_time,
							sync_period,				// periodic timer ( millisecs )
							NULL,						// completion routine
							NULL,						// completion routine argument
							FALSE );					// do not resume from suspended power mode
		clock.start();


		UINT rc = ProcessorNode< T_SOURCE_DATA, T_SINK_DATA >::loop();
		

		CancelWaitableTimer( timer );

		return rc;
	}

};



#endif
