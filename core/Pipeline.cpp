
#include "pipeline.h"
#include "..\gui\OmniApp2.h"
#include "..\tools\timer.h"
#include "..\tools\pview\pview.h"
#include "CfgParameters.h"





#define HIDE_MOUSE_CURSOR()				\
				ShowCursor( FALSE );	\
				POINT _pt;				\
				GetCursorPos( &_pt );	\
				SetCursorPos(0,0);		

#define SHOW_MOUSE_CURSOR()				\
				ShowCursor( TRUE );		\
				SetCursorPos( _pt.x, _pt.y );





Pipeline::Pipeline()
{
	is_initialising = false;
	is_suspended = false;

	do_stop = CreateEvent(	NULL, 
							false,		// automatic reset
							false,		// initially NOT signalled
							NULL );


	node_reader = 0;
	node_pal = 0;
	node_sync = 0;
	node_iconvw = 0;
	node_iconvw3 = 0;
	node_iconvw4 = 0;
	node_iconvw5 = 0;

	node_viewer = 0;
	node_viewer3 = 0;
	node_viewer4 = 0;
	node_viewer5 = 0;
	
	for ( int k = 0; k < MAX_PERSPECTIVES; ++k )
	{
		node_pers_viewer[ k ] = 0;
		node_pers_dewarper[ k ] = 0;
		node_pers_icon[ k ] = 0;
	}
	num_perspectives = 0;

	for ( int k = 0; k < MAX_PANORAMAS; ++k )
	{
		node_pano_dewarper[ k ] = 0;
		node_pano_viewer[ k ] = 0;
		node_pano_icon[ k ] = 0;
	}
	num_panoramas = 0;

	node_bkgSubtracter = 0;
	node_blobTracker = 0;
	node_colourTracker = 0;
	node_cameraController = 0;
	node_history = 0;
	node_gray = 0;

	// used by pview tools when displaying thread information
	registerThreadName( this->thread->m_nThreadID, "pipeline" );


	icon_update_rate = -1;					// rate at which icons are refreshed

	initGUI();
}




void Pipeline::initGUI()
{
	CMainApp* theApp = (CMainApp*) AfxGetApp();
	assert( theApp );


	//TO DO: we should really clean and re-create the GUI windows and stuff
	if ( theApp->cam_view->numWindows() > 0 )
		return;

	
	CfgParameters* cfg = CfgParameters::instance();


	// initial configuration
	theApp->cam_view->newFolder( " Omnidirectional", 0, -1, false );
	theApp->cam_view->newFolder( " Panoramas", 1, 0 );
	theApp->cam_view->newFolder( " Perspectives", 1, 0 );
	theApp->cam_view->newFolder( " Other", 1, 0, false );	


	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "OmniCam #1" ),
				new CFloatingOmniImageView( "OmniCam #1" ),
				0 );

	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "Panorama #1" ),
				new CFloatingPanoramaView( "Panorama #1" ),
				1, 1.75, 0.75 );

	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "Perspective #1" ),
				new CFloatingPerspectiveView( "Perspective #1" ),
				2 );

	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "Bkg Subtraction #1" ),
				new CFloatingImageView( "Background Subtraction #1" ),
				3 );

	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "Bkg Subtraction #2" ),
				new CFloatingImageView( "Background Subtraction #2" ),
				3 );

	theApp->cam_view->newVideoWindow( 
				new CStaticImageView( "Bkg Subtraction #3" ),
				new CFloatingImageView( "Background Subtraction #3" ),
				3 );


	theApp->cam_view->OnInitialUpdate();


	HIDE_MOUSE_CURSOR();
	{
		theApp->cam_view->openFloatingWindow( 0 );
		theApp->cam_view->getVideoWindow( 0 )->ShowWindow( SW_HIDE );

		theApp->cam_view->openFloatingWindow( 1 );
		theApp->cam_view->getVideoWindow( 1 )->ShowWindow( SW_HIDE );
		
		theApp->cam_view->openFloatingWindow( 2 );
		theApp->cam_view->getVideoWindow( 2 )->ShowWindow( SW_HIDE );

		theApp->cam_view->openFloatingWindow( 3 );
		theApp->cam_view->getVideoWindow( 3 )->ShowWindow( SW_HIDE );

		theApp->cam_view->openFloatingWindow( 4 );
		theApp->cam_view->getVideoWindow( 4 )->ShowWindow( SW_HIDE );

		theApp->cam_view->openFloatingWindow( 5 );
		theApp->cam_view->getVideoWindow( 5 )->ShowWindow( SW_HIDE );
	}
	SHOW_MOUSE_CURSOR();
}




Pipeline::~Pipeline()
{
	// used by pview tools when displaying thread information
	registerThreadName( this->thread->m_nThreadID, "pipeline" );

	// in case we are still running...
	end( INFINITE );

	CloseHandle( do_stop );
}




void Pipeline::doCalibration()
{
	CfgParameters* params = CfgParameters::instance();


	CMainApp* theApp = (CMainApp*) AfxGetApp();
	assert( theApp );


	if ( params->doAutoCalibration() )
	{
		LOG( Msg::CALIBRATION_STARTED );


		bool calibration_result = false;


		HIDE_MOUSE_CURSOR();
		{
			((CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) )->DisableBorders();
			theApp->cam_view->getVideoWindow( 0 )->ShowWindow( SW_SHOW );
			theApp->cam_view->getVideoWindow( 1 )->ShowWindow( SW_HIDE );
			theApp->cam_view->getVideoWindow( 2 )->ShowWindow( SW_HIDE );
		}
		SHOW_MOUSE_CURSOR();


		
		node_reader = new ReadJpegSequence_Type
					(	"OMNI#1-READER", 
						params->getImageSource().operator LPCTSTR(), 
						&MemPool, 
						params->getStartFrame(), 
						params->getStartFrame() );	// one frame only for calibration

		if ( params->needAspectCorrection() )
			node_pal  = new CorrectAspectRatio_Type
						(	"PAL#1",
							&MemPool,
							params->getInAspectX(), params->getInAspectY(), 
							params->getOutAspectX(), params->getOutAspectY(),
							IPL_INTER_LINEAR );

		node_gray = new ToGray_Type
					( "COLOUR2GRAY#1", &MemPool );

		node_threshold = new Threshold_Type
						 ( "THRESHOLD#1", &MemPool );

		node_edge = new EdgeDetector_Type
					( "EDGE DETECTION#1", &MemPool );

		node_boundary = new BoundaryDetector_Type
					  ( "BOUNDARY DETECTION#1", &calibration_result, &MemPool );

		node_slideshow = new SlideshowViewer_Type
						( "OMNI#1-viewer", &MemPool, 
						  (CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) );

		node_iconvw = new OmniIconViewer_Type
					( "OMNI#1-icon_viewer", &MemPool, theApp->cam_view->getVideoWindowIcon( 0 ) );

		node_viewer = new OmniViewer_Type
						( "OMNI#1-viewer", &MemPool, 
						  (CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) );
		
		
		if ( node_pal )
		{
			node_reader->addSink( node_pal );
			node_pal->addSink( node_viewer );
			node_pal->addSink( node_iconvw );
			node_pal->addSink( node_gray );
		}
		else
		{
			node_reader->addSink( node_viewer );
			node_reader->addSink( node_iconvw );
			node_reader->addSink( node_gray );
		}
		node_gray->addSink( node_threshold );
		node_gray->addSink( node_slideshow );
		node_threshold->addSink( node_slideshow );
		node_threshold->addSink( node_edge );
		node_edge->addSink( node_slideshow );
		node_edge->addSink( node_boundary );
		if ( node_pal )
			node_pal->addSink( node_boundary );
		else
			node_reader->addSink( node_boundary );
		node_boundary->addSink( node_slideshow );


		const int MAX_HANDLES = 10;
		HANDLE handles[ MAX_HANDLES ];
		int nodes = 0;

		handles[ nodes++ ] = node_reader->handle();
		if ( node_pal ) handles[ nodes++ ] = node_pal->handle();
		handles[ nodes++ ] = node_slideshow->handle();
		handles[ nodes++ ] = node_gray->handle();
		handles[ nodes++ ] = node_threshold->handle();
		handles[ nodes++ ] = node_edge->handle();
		handles[ nodes++ ] = node_boundary->handle();
		handles[ nodes++ ] = node_viewer->handle();
		handles[ nodes++ ] = node_iconvw->handle();
		assert( nodes < MAX_HANDLES );


		node_reader->resume();
		if ( node_pal ) node_pal->resume();

		node_slideshow->resume();
		node_gray->resume();
		node_threshold->resume();
		node_edge->resume();
		node_boundary->resume();
		node_viewer->resume();
		node_iconvw->resume();


		DWORD rc = WaitForMultipleObjects( nodes, handles, true, INFINITE );


		delete node_reader;
		if ( node_pal ) delete node_pal;
		delete node_slideshow;
		delete node_gray;
		delete node_threshold;
		delete node_edge;
		delete node_boundary;
		delete node_viewer;
		delete node_iconvw;

		node_reader = 0;
		node_pal = 0;
		node_slideshow = 0;
		node_gray = 0;
		node_threshold = 0;
		node_edge = 0;
		node_boundary = 0;
		node_viewer = 0;
		node_iconvw = 0;


		if ( ! calibration_result )
		{
			CLOG( "Mirror Boundary not detected! - Calibration Failed!", 0x0000ee );
			CLOG( "Mirror parameters set to default values. Calibration parameters can be specified in the INI file.", 0x0000ee ); 
		}

	
		((CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) )->reset();
		((CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) )->EnableBorders();
		theApp->cam_view->getVideoWindowIcon( 0 )->reset();
	}



	// make sure we have a valid image ROI
	{
		if ( params->getRoiMinX() == -1 || 
			 params->getRoiMinY() == -1 ||
			 params->getRoiMaxX() == -1 ||
			 params->getRoiMaxY() == -1 )
		{
			char* path = new char[ MAX_PATH ] ;
			sprintf( path, params->getImageSource(), params->getStartFrame() );

			IplImage* tmp = cvLoadImage( path );

			if ( tmp )
			{
				params->getRoiMinX() = 0;
				params->getRoiMinY() = 0;
				params->getRoiMaxX() = tmp->width -1;
				params->getRoiMaxY() = tmp->height -1;

				cvReleaseImage( &tmp );
			}
			else
			{
				AfxMessageBox(	"No image frames found!\n\n"
								"Please check that path is correctly specified in the INI file,\n"
								"or that the start frame (if specified) is correct.", MB_ICONERROR );
				return;
			}

			delete[] path;
		}
	}

	CLOG( "Mirror centre = ("
			<< ( params->getRoiMinX() + params->getMirrorCentreX() ) * params->getAspectRatioX() << ","
			<< ( params->getRoiMinY() + params->getMirrorCentreY() ) * params->getAspectRatioY() << ")  boundary radius = "
			<< params->getMirrorBoundary() << "  mirror focal length = "
			<< params->getMirrorFocalLen() << "  image ROI = ("
			<< params->getRoiMinX() << "," << params->getRoiMinY() << ") to ("
			<< params->getRoiMaxX() << "," << params->getRoiMaxY() << ")",
			0xff0000 );


	params->constructMask();
	assert( params->getMask() );


	LOG( Msg::CALIBRATION_FINISHED );
	Sleep( 2000 );
}




UINT Pipeline::loop()
{
	const int MAX_HANDLES = 50;
	HANDLE handles[ MAX_HANDLES ];
	int numHandles = 0;
	int numNodes = 0;

	CfgParameters* cfg = CfgParameters::instance();


	is_initialising = true;
	{
		safe_state.Lock();
		{
			doCalibration();

			initNodes();


			handles[ numHandles++ ] = do_stop;
			handles[ numHandles++ ] = node_reader->handle();
			if ( node_pal )
				handles[ numHandles++ ] = node_pal->handle();
			handles[ numHandles++ ] = node_sync->handle();
			for ( int k = 0; k < num_panoramas; ++k )
			{
				handles[ numHandles++ ] = node_pano_dewarper[ k ]->handle();
				handles[ numHandles++ ] = node_pano_viewer[ k ]->handle();
				handles[ numHandles++ ] = node_pano_icon[ k ]->handle();
			}
			handles[ numHandles++ ] = node_bkgSubtracter->handle();
			
			if ( cfg->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
				handles[ numHandles++ ] = node_blobTracker->handle();
			else
				handles[ numHandles++ ] = node_colourTracker->handle();

			handles[ numHandles++ ] = node_cameraController->handle();
			handles[ numHandles++ ] = node_history->handle();
			handles[ numHandles++ ] = node_gray->handle();
			handles[ numHandles++ ] = node_iconvw->handle();
			handles[ numHandles++ ] = node_iconvw3->handle();
			handles[ numHandles++ ] = node_iconvw4->handle();
			handles[ numHandles++ ] = node_iconvw5->handle();
			handles[ numHandles++ ] = node_viewer->handle();
			handles[ numHandles++ ] = node_viewer3->handle();
			handles[ numHandles++ ] = node_viewer4->handle();
			handles[ numHandles++ ] = node_viewer5->handle();
			for ( int k = 0; k < num_perspectives; ++k )
			{
				handles[ numHandles++ ] = node_pers_viewer[ k ]->handle();
				handles[ numHandles++ ] = node_pers_dewarper[ k ]->handle();
				handles[ numHandles++ ] = node_pers_icon[ k ]->handle();
			}
			assert( numHandles <= MAXIMUM_WAIT_OBJECTS );
			assert( numHandles < MAX_HANDLES );
			numNodes = numHandles -1;


			LOG( "Started processing..." );



			// start the nodes of the pipeline...
			node_iconvw->resume();
			node_iconvw3->resume();
			node_iconvw4->resume();
			node_iconvw5->resume();
			node_viewer->resume();
			node_viewer3->resume();
			node_viewer4->resume();
			node_viewer5->resume();
			for ( int k = 0; k < num_perspectives; ++k )
			{
				node_pers_viewer[ k ]->resume();
				node_pers_dewarper[ k ]->resume();
				node_pers_icon[ k ]->resume();
			}
			for ( int k = 0; k < num_panoramas; ++k )
			{
				node_pano_dewarper[ k ]->resume();
				node_pano_viewer[ k ]->resume();
				node_pano_icon[ k ]->resume();
			}
			
			node_bkgSubtracter->resume();
			
			if ( cfg->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
				node_blobTracker->resume();
			else
				node_colourTracker->resume();

			node_cameraController->resume();
			node_history->resume();
			node_gray->resume();
			if ( node_pal )
				node_pal->resume();
			node_sync->resume();


			CMainApp* theApp = (CMainApp*) AfxGetApp();
			assert( theApp );

			HIDE_MOUSE_CURSOR();
			{
				bool visible;

				cfg->readWindowState( theApp->cam_view->getVideoWindow( 0 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 0 )->ShowWindow( SW_SHOW );

				cfg->readWindowState( theApp->cam_view->getVideoWindow( 1 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 1 )->ShowWindow( SW_SHOW );
				
				cfg->readWindowState( theApp->cam_view->getVideoWindow( 2 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 2 )->ShowWindow( SW_SHOW );
	
				cfg->readWindowState( theApp->cam_view->getVideoWindow( 3 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 3 )->ShowWindow( SW_SHOW );
	
				cfg->readWindowState( theApp->cam_view->getVideoWindow( 4 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 4 )->ShowWindow( SW_SHOW );
	
				cfg->readWindowState( theApp->cam_view->getVideoWindow( 5 )->getName(), visible );
				if ( visible )
					theApp->cam_view->getVideoWindow( 5 )->ShowWindow( SW_SHOW );
			}
			SHOW_MOUSE_CURSOR();


			total_time = 0;
			clock.start();


			node_reader->resume();
		}
		safe_state.Unlock();
	}
	is_initialising = false;


	// wait till told to stop or pipeline nodes finish their processing...
	while( true )
	{
		DWORD rc = WaitForMultipleObjects( numHandles, handles, false, INFINITE );

		// we received a signal to close down...
		if ( rc == WAIT_OBJECT_0 )
		{
			safe_state.Lock();
			{
				Sleep( 100 );		// yield some time...

				// killing the source (node_reader) of the pipeline;  the
				// others will die by a dominoe effect
				node_reader->end( INFINITE );
				for ( int k = 0; k < num_panoramas; ++k )
					node_pano_dewarper[ k ]->wait( INFINITE );
				for ( int k = 0; k < num_perspectives; ++k )
					node_pers_dewarper[ k ]->wait( INFINITE );
				node_iconvw->wait( INFINITE );	
				node_iconvw3->wait( INFINITE );
				node_iconvw4->wait( INFINITE );
				node_iconvw5->wait( INFINITE );
				node_viewer->wait( INFINITE );			
				node_viewer3->wait( INFINITE );
				node_viewer4->wait( INFINITE );
				node_viewer5->wait( INFINITE );
				for ( int k = 0; k < num_perspectives; ++k )
				{
					node_pers_viewer[ k ]->wait( INFINITE );
					node_pers_icon[ k ]->wait( INFINITE );
				}
				for ( int k = 0; k < num_panoramas; ++k )
				{
					node_pano_viewer[ k ]->wait( INFINITE );
					node_pano_icon[ k ]->wait( INFINITE );
				}

				if ( cfg->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
					node_blobTracker->wait( INFINITE );
				else
					node_colourTracker->wait( INFINITE );

				node_cameraController->wait( INFINITE );
				node_history->wait( INFINITE );
				node_bkgSubtracter->wait( INFINITE );
				node_gray->wait( INFINITE );
				node_sync->wait( INFINITE );
				if ( node_pal )
					node_pal->wait( INFINITE );

				killNodes();
			}
			safe_state.Unlock();

			return RC_ABORTED;
		}

		// check if all the nodes finished their processing...
		if ( WaitForMultipleObjects( numNodes, handles+1, true, 0 ) == WAIT_OBJECT_0 )
		{
			safe_state.Lock();
			{
				clock.stop();

				CfgParameters* params = CfgParameters::instance();

				Beep( 600, 200 );
				
				LOG( "Finished processing." );
				double time_taken = total_time + clock.duration();
				
				long frames_proc = node_reader->getFrameNumber() - params->getStartFrame();
				if ( frames_proc < 0 )
					frames_proc = 0;

				CLOG( "Total frames processed = " << frames_proc, 0xff0000 );
				CLOG( "Total processing time = " << time_taken << " sec.", 0xff0000 );
				CLOG( frames_proc / time_taken << " frames per second", 0xff0000 );


				// an error?
				if ( frames_proc <= 0 )
				{
					if ( node_reader->getReturnCode() == RC_ERROR )
						AfxMessageBox( Msg::ERROR_READING_SOURCE );
				}


				killNodes();
			}
			safe_state.Unlock();

			return RC_SUCCESS;
		}

		// else some node finished prematurely or is not running.
		// yield processor time, to avoid having a tight loop...
		Sleep( 800 );
	}
}




void Pipeline::initNodes()
{
	CfgParameters* params = CfgParameters::instance();


	CMainApp* theApp = (CMainApp*) AfxGetApp();
	assert( theApp );



	// icons refresh every 2 seconds
	icon_update_rate = (int)( params->getFps() / (float) params->getReadStep() * 2 );



	// ------ configuration ---------------
	// this is the region to get from the JPEG file
	CvRect roi;
	if ( params->isUsingROI() )
	{
		int roi_x0 = params->getRoiMinX();
		int roi_y0 = params->getRoiMinY();
		int roi_w  = params->getRoiMaxX() - roi_x0 +1;
		int roi_h  = params->getRoiMaxY() - roi_y0 +1;
		roi = cvRect( roi_x0, roi_y0, roi_w, roi_h );
	}


	node_reader = new ReadJpegSequence_Type
				(	"OMNI#1-READER", 
					params->getImageSource().operator LPCTSTR(), 
					&MemPool, 
					params->getStartFrame(), params->getEndFrame(), 
					params->getReadStep(), params->isUsingROI() ? &roi : 0 );

	if ( params->needAspectCorrection() )
		node_pal  = new CorrectAspectRatio_Type
					(	"PAL#1",
						&MemPool,
						params->getInAspectX(), params->getInAspectY(), 
						params->getOutAspectX(), params->getOutAspectY(),
						IPL_INTER_LINEAR );

	node_sync = new Sync_Type
				(	"SYNC#1",
					1000 / params->getFps() * params->getReadStep(),
					((CMainFrame*) AfxGetApp()->m_pMainWnd),
					params->getFps() / params->getReadStep() * 3 );

	node_iconvw = new OmniIconViewer_Type
				( "OMNI#1-icon_viewer", &MemPool, theApp->cam_view->getVideoWindowIcon( 0 ) );
	node_iconvw->changeRate( icon_update_rate );
	node_iconvw->high_priority = false;

	node_viewer = new OmniViewer_Type
				( "OMNI#1-viewer", &MemPool, 
				  (CFloatingOmniImageView*) theApp->cam_view->getVideoWindow( 0 ) );
	node_viewer->high_priority = false;

	node_pano_dewarper[ 0 ] = new PanoramicDewarper_Type
				( "PANORAMIC_DEWARPER#1", &MemPool );
	node_pano_dewarper[ 0 ]->high_priority = false;

	node_pers_dewarper[ 0 ] = new PerspectiveDewarper_Type
				( "PERSPECTIVE_DEWARPER#1", &MemPool );
	node_pers_dewarper[ 0 ]->high_priority = false;

	node_bkgSubtracter = new BkgSubtracter_Type
				( "BKG_SUBTRACTER", params->getBackgroundInitFrames(), &MemPool );

	if ( params->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
	{
		node_blobTracker = new BlobTracker_Type
					( "BLOB_TRACKER", &MemPool );
	}
	else
	{
		node_colourTracker = new ColourTracker_Type
					( "COLOUR_TRACKER", &MemPool );
	}

	node_cameraController = new CameraController_Type
				( "VIRTUAL_CAMERA_CONTROLLER", &MemPool );
	node_cameraController->high_priority = false;

	node_history = new HistoryNode_Type
				( "HISTORY_NODE", &MemPool );

	node_pers_icon[ 0 ] = new PersIconViewer_Type
				( "PERSPECTIVE#1-icon_viewer", &MemPool, theApp->cam_view->getVideoWindowIcon( 2 ) );
	node_pers_icon[ 0 ]->changeRate( icon_update_rate );
	node_pers_icon[ 0 ]->high_priority = false;

	node_iconvw3 = new BkgIconViewer_Type
				( "BKG SUBTRACTION#1-icon_viewer", &MemPool, 
				  theApp->cam_view->getVideoWindowIcon( 3 ) );
	node_iconvw3->changeRate( icon_update_rate );
	node_iconvw3->high_priority = false;

	node_iconvw4 = new BkgIconViewer_Type
				( "BKG SUBTRACTION#2-icon_viewer", &MemPool, 
				  theApp->cam_view->getVideoWindowIcon( 4 ) );
	node_iconvw4->changeRate( icon_update_rate );
	node_iconvw4->high_priority = false;

	node_iconvw5 = new BkgIconViewer_Type
				( "BKG SUBTRACTION#3-icon_viewer", &MemPool, 
				  theApp->cam_view->getVideoWindowIcon( 5 ) );
	node_iconvw5->changeRate( icon_update_rate );
	node_iconvw5->high_priority = false;

	node_pano_icon[ 0 ] = new PanoIconViewer_Type
				( "PANORAMIC#1-icon_viewer", &MemPool, theApp->cam_view->getVideoWindowIcon( 1 ) );
	node_pano_icon[ 0 ]->changeRate( icon_update_rate );
	node_pano_icon[ 0 ]->high_priority = false;

	node_pano_viewer[ 0 ] = new PanoViewer_Type
				( "PANORAMA#1-viewer", &MemPool, 
				  (CFloatingPanoramaView*) theApp->cam_view->getVideoWindow( 1 ) );
	( (CFloatingPanoramaView*) theApp->cam_view->getVideoWindow( 1 ) )->setNodeToken( 0 );
	node_pano_viewer[ 0 ]->high_priority = false;

	node_viewer3 = new BkgSubtractViewer_Type
				( "BKG SUBTRACTION#1-viewer", &MemPool, theApp->cam_view->getVideoWindow( 3 ) );
	node_viewer3->high_priority = false;

	node_viewer4 = new BkgSubtractViewer_Type
				( "BKG SUBTRACTION#2-viewer", &MemPool, theApp->cam_view->getVideoWindow( 4 ) );
	node_viewer4->high_priority = false;

	node_viewer5 = new BkgSubtractViewer_Type
				( "BKG SUBTRACTION#s-viewer", &MemPool, theApp->cam_view->getVideoWindow( 5 ) );
	node_viewer5->high_priority = false;

	node_pers_viewer[ 0 ] = new PersViewer_Type
				( "PERSPECTIVE#1-viewer", &MemPool, 
				  (CFloatingPerspectiveView*) theApp->cam_view->getVideoWindow( 2 ) );
	( (CFloatingPerspectiveView*) theApp->cam_view->getVideoWindow( 2 ) )->setNodeToken( 0 );
	node_pers_viewer[ 0 ]->high_priority = false;

	node_gray = new ToGray_Type
				( "COLOUR2GRAY#1", &MemPool );

	num_perspectives = 1;
	num_panoramas = 1;


	// display some info
	LOG( "" );
	LOG( "Source for input video stream [" << node_reader->my_name.c_str() 
		 << "] is:  " << params->getImageSource().operator LPCTSTR() );
	char tmp[20] = "n/a";
	if ( params->getEndFrame() != -1 )
		sprintf( tmp, "%d", params->getEndFrame() );
	LOG( "   Start frame: " << params->getStartFrame() << "\tEnd frame: " << tmp
		 << "\tFps: " << params->getFps() << "\tActual read rate: " << params->getReadStep() );
	LOG( "" );


	// used by pview tools when displaying thread information
	registerThreadName( node_reader->threadID(),		node_reader->my_name );
	registerThreadName( node_sync->threadID(),			node_sync->my_name );
	for ( int k = 0; k < num_panoramas; ++k )
	{
		registerThreadName( node_pano_dewarper[k]->threadID(),	node_pano_dewarper[k]->my_name );
		registerThreadName( node_pano_viewer[k]->threadID(),	node_pano_viewer[k]->my_name );
		registerThreadName( node_pano_icon[k]->threadID(),		node_pano_icon[k]->my_name );
	}
	if ( node_pal )
		registerThreadName( node_pal->threadID(),		node_pal->my_name );
	registerThreadName( node_bkgSubtracter->threadID(),	node_bkgSubtracter->my_name );
	
	if ( params->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
		registerThreadName( node_blobTracker->threadID(),	node_blobTracker->my_name );
	else
		registerThreadName( node_colourTracker->threadID(),	node_colourTracker->my_name );

	registerThreadName( node_cameraController->threadID(),	node_cameraController->my_name );
	registerThreadName( node_history->threadID(),		node_history->my_name );
	registerThreadName( node_gray->threadID(),			node_gray->my_name );
	registerThreadName( node_viewer->threadID(),		node_viewer->my_name );
	registerThreadName( node_viewer3->threadID(),		node_viewer3->my_name );
	registerThreadName( node_viewer4->threadID(),		node_viewer4->my_name );
	registerThreadName( node_viewer5->threadID(),		node_viewer5->my_name );
	for ( int k = 0; k < num_perspectives; ++k )
	{
		registerThreadName( node_pers_viewer[k]->threadID(),	node_pers_viewer[k]->my_name );
		registerThreadName( node_pers_dewarper[k]->threadID(),	node_pers_dewarper[k]->my_name );
		registerThreadName( node_pers_icon[k]->threadID(),		node_pers_icon[k]->my_name );
	}
	registerThreadName( node_iconvw->threadID(),		node_iconvw->my_name );
	registerThreadName( node_iconvw3->threadID(),		node_iconvw3->my_name );
	registerThreadName( node_iconvw4->threadID(),		node_iconvw4->my_name );
	registerThreadName( node_iconvw5->threadID(),		node_iconvw5->my_name );



	if ( node_pal )
	{
		node_reader->addSink( node_pal );
//			node_pal->addSink( node_sync );
		node_pal->addSink( node_iconvw );
		node_pal->addSink( node_viewer );
		node_pal->addSink( node_pano_dewarper[0] );
		node_pal->addSink( node_pers_dewarper[0] );
		node_pal->addSink( node_bkgSubtracter );
		node_pal->addSink( node_history );
	}
	else
	{
//			node_reader->addSink( node_sync );
		node_reader->addSink( node_iconvw );
		node_reader->addSink( node_viewer );
		node_reader->addSink( node_pano_dewarper[0] );
		node_reader->addSink( node_pers_dewarper[0] );
		node_reader->addSink( node_bkgSubtracter );
		node_reader->addSink( node_history );
	}
	node_pano_dewarper[0]->addSink( node_pano_icon[0] );
	node_pano_dewarper[0]->addSink( node_pano_viewer[0] );
	node_pers_dewarper[0]->addSink( node_pers_icon[0] );
	node_pers_dewarper[0]->addSink( node_pers_viewer[0] );

	if ( params->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
	{
		node_bkgSubtracter->addSink( node_blobTracker );
		node_bkgSubtracter->addSink( node_blobTracker );
		node_bkgSubtracter->addSink( node_blobTracker );
		node_bkgSubtracter->addSink( node_blobTracker );
		node_bkgSubtracter->addSink( node_blobTracker );

		node_blobTracker->addSink( node_iconvw3 );
		node_blobTracker->addSink( node_viewer3 );
		node_blobTracker->addSink( node_cameraController );
		node_blobTracker->addSink( node_history );
	}
	else
	{
		node_bkgSubtracter->addSink( node_colourTracker );
		node_bkgSubtracter->addSink( node_colourTracker );
		node_bkgSubtracter->addSink( node_colourTracker );
		node_bkgSubtracter->addSink( node_colourTracker );
		node_bkgSubtracter->addSink( node_colourTracker );

		node_colourTracker->addSink( node_iconvw3 );
		node_colourTracker->addSink( node_viewer3 );
		node_colourTracker->addSink( node_cameraController );
		node_colourTracker->addSink( node_history );
	}

	node_cameraController->addSink( node_pers_dewarper[0] );



	node_pano_dewarper[0]->initMappingFov( 360.0, 50.0,
									 params->getMirrorCentreX(), params->getMirrorCentreY(),
									 params->getMirrorFocalLen(),
									 params->isCameraInverted(),
									 params->getMirrorVfov(),
									 true,
									 1, 0, 0, -10 );



	node_pers_dewarper[0]->initMapping( params->getPerspectiveViewSizeX(), 
										params->getPerspectiveViewSizeY(),
										params->getMirrorCentreX(), params->getMirrorCentreY(),
										params->getMirrorFocalLen(),
										params->isCameraInverted(),
										params->getMirrorVfov(),
										true,
										1, 0, 0, -10 );


	SetThreadPriority( this->handle(), THREAD_PRIORITY_BELOW_NORMAL );
}




DWORD Pipeline::resume()
{
	is_suspended = false;

	if ( ! has_started )
	{
		has_started = true;
		
		clock.start();
		return thread->ResumeThread();
	}

	if ( node_reader )
	{
		clock.start();
		return node_reader->resume();
	}

	return 0xFFFFFFFF;
}




DWORD Pipeline::suspend()
{
	// to suspend the pipeline, we only need to suspend the first
	// node (node_reader in this case). The other nodes will be
	// blocked (so consuming no CPU) while waiting for data. This
	// also ensures that the rest of the nodes have finish off 
	// their processing, so suspension is synched on the same frame.

	if ( node_reader )
	{
		is_suspended = true;
		DWORD rc = node_reader->suspend();

		clock.stop();
		total_time += clock.duration();

		return rc;
	}

	return 0xFFFFFFFF;
}




void Pipeline::killNodes()
{
	CfgParameters* cfg = CfgParameters::instance();

	
	// used by pview tools when displaying thread information
	registerThreadName( node_reader->threadID(),		"" );	// unregister all, since thread IDs can be recylced by Windows
	registerThreadName( node_sync->threadID(),			"" );		
	for ( int k = 0; k < num_panoramas; ++k )
	{
		registerThreadName( node_pano_dewarper[k]->threadID(),	"" );
		registerThreadName( node_pano_viewer[k]->threadID(),	"" );
		registerThreadName( node_pano_icon[k]->threadID(),		"" );
	}
	if ( node_pal )
		registerThreadName( node_pal->threadID(),		"" );
	registerThreadName( node_bkgSubtracter->threadID(),	"" );

	if ( cfg->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
		registerThreadName( node_blobTracker->threadID(),	"" );
	else
		registerThreadName( node_colourTracker->threadID(),	"" );

	registerThreadName( node_cameraController->threadID(), "" );
	registerThreadName( node_history->threadID(),		"" );
	registerThreadName( node_gray->threadID(),			"" );
	registerThreadName( node_viewer->threadID(),		"" );
	registerThreadName( node_viewer3->threadID(),		"" );
	registerThreadName( node_viewer4->threadID(),		"" );
	registerThreadName( node_viewer5->threadID(),		"" );
	for ( int k = 0; k < num_perspectives; ++k )
	{
		registerThreadName( node_pers_viewer[k]->threadID(),	"" );
		registerThreadName( node_pers_dewarper[k]->threadID(),	"" );
		registerThreadName( node_pers_icon[k]->threadID(),		"" );
	}
	registerThreadName( node_iconvw->threadID(),		"" );
	registerThreadName( node_iconvw3->threadID(),		"" );
	registerThreadName( node_iconvw4->threadID(),		"" );
	registerThreadName( node_iconvw5->threadID(),		"" );

	
	for ( int k = 0; k < num_panoramas; ++k )
	{
		delete node_pano_dewarper[k];
		delete node_pano_viewer[k];
		delete node_pano_icon[k];
	}
	
	if ( cfg->getTrackingMethod() == CfgParameters::BLOB_TRACKING_METHOD )
		delete node_blobTracker;
	else
		delete node_colourTracker;

	delete node_cameraController;
	delete node_history;
	delete node_bkgSubtracter;
	delete node_gray;
	delete node_viewer3;
	delete node_viewer4;
	delete node_viewer5;
	for ( int k = 0; k < num_perspectives; ++k )
	{
		delete node_pers_viewer[k];
		delete node_pers_dewarper[k];
		delete node_pers_icon[k];
	}
	delete node_viewer;
	delete node_iconvw;
	delete node_iconvw3;
	delete node_iconvw4;
	delete node_iconvw5;
	delete node_sync;
	delete node_reader;
	if ( node_pal )
		delete node_pal;

	for ( int k = 0; k < num_panoramas; ++k )
	{
		node_pano_dewarper[k] = 0;
		node_pano_viewer[k] = 0;
		node_pano_icon[k] = 0;
	}
	node_bkgSubtracter = 0;
	node_blobTracker = 0;
	node_history = 0;
	node_colourTracker = 0;
	node_cameraController = 0;
	node_gray = 0;
	node_viewer = 0;
	node_viewer3 = 0;
	node_viewer4 = 0;
	node_viewer5 = 0;
	for ( int k = 0; k < num_perspectives; ++k )
	{
		node_pers_viewer[k] = 0;
		node_pers_dewarper[k] = 0;
		node_pers_icon[k] = 0;
	}
	node_iconvw = 0;
	node_iconvw3 = 0;
	node_iconvw4 = 0;
	node_iconvw5 = 0;
	node_sync = 0;
	node_reader = 0;
	node_pal = 0;
}




void Pipeline::OnFolderButton2()
{
	// create a new panorama view

	CMainApp* theApp = (CMainApp*) AfxGetApp();
	assert( theApp );

	CfgParameters* params = CfgParameters::instance();

	if ( num_panoramas == MAX_PANORAMAS )
	{
		CLOG( Msg::PANORAMA_LIMIT_EXCEEDED, 0x0000ff );
		return;
	}


	char tmp[50];


	// add new GUI window for this view
	sprintf( tmp, "Panorama #%d", num_panoramas+1 );
	int video_win_num = theApp->cam_view->newVideoWindow( 
										new CStaticImageView( tmp ),
										new CFloatingPanoramaView( tmp ),
										1, 1.75, 0.75 );

	if ( video_win_num == -1 )
	{
		CLOG( Msg::TOTAL_WINDOW_LIMIT_EXCEEDED, 0x0000ff );
		return;
	}


	theApp->cam_view->OnInitialUpdate();


	safe_state.Lock();
	{
		// add viewer node
		sprintf( tmp, "PANORAMA#%d-viewer", num_panoramas+1 );
		node_pano_viewer[ num_panoramas ] = new PanoViewer_Type
					( tmp, &MemPool, 
					  (CFloatingPanoramaView*) theApp->cam_view->getVideoWindow( video_win_num ) );


		// add viewer icon node
		sprintf( tmp, "PANORAMA#%d-icon_viewer", num_panoramas+1 );
		node_pano_icon[ num_panoramas ] = new PanoIconViewer_Type
					( tmp, &MemPool, 
					  theApp->cam_view->getVideoWindowIcon( video_win_num ) );
		node_pano_icon[ num_panoramas ]->changeRate( icon_update_rate );


		// add dewarper node
		sprintf( tmp, "PANORAMIC_DEWARPER#%d-viewer", num_panoramas+1 );
		node_pano_dewarper[ num_panoramas ] = new PanoramicDewarper_Type
					( tmp, &MemPool );


		// link video window's controls to dewarper node
		( (CFloatingPanoramaView*) theApp->cam_view->getVideoWindow( video_win_num ) )->setNodeToken( num_panoramas );


		// init dewarper
		// TO DO: in menu, make an option to set preferences for panoramic, perspective views (e.g. default size is 320x200, etc.)
		node_pano_dewarper[ num_panoramas ]->initMappingFov( 360.0, 50.0,
										 params->getMirrorCentreX(), params->getMirrorCentreY(),
										 params->getMirrorFocalLen(),
										 params->isCameraInverted(),
										 params->getMirrorVfov(),
										 true,
										 1, 45 * num_panoramas, 0, -10 );

		// register the threads
		registerThreadName( node_pano_viewer[ num_panoramas ]->threadID(),	
							node_pano_viewer[ num_panoramas ]->my_name );
		registerThreadName( node_pano_dewarper[ num_panoramas ]->threadID(),	
							node_pano_dewarper[ num_panoramas ]->my_name );


		// add to pipeline
		node_pano_dewarper[ num_panoramas ]->addSink( node_pano_icon[ num_panoramas ] );
		node_pano_dewarper[ num_panoramas ]->addSink( node_pano_viewer[ num_panoramas ] );

		if ( node_pal )
			node_pal->addSink( node_pano_dewarper[ num_panoramas ] );
		else
			node_reader->addSink( node_pano_dewarper[ num_panoramas ] );


		// start the nodes
		node_pano_viewer[ num_panoramas ]->resume();
		node_pano_dewarper[ num_panoramas ]->resume();
		node_pano_icon[ num_panoramas ]->resume();

		num_panoramas++;
	}
	safe_state.Unlock();
}




void Pipeline::OnNewPerspectiveView()
{
	// create a new perspective view

	CMainApp* theApp = (CMainApp*) AfxGetApp();
	assert( theApp );

	CfgParameters* params = CfgParameters::instance();

	if ( num_perspectives == MAX_PERSPECTIVES )
	{
		CLOG( Msg::PERSPECTIVE_LIMIT_EXCEEDED, 0x0000ff );
		return;
	}


	char tmp[50];


	// add new GUI window for this view
	sprintf( tmp, "Perspective #%d", num_perspectives+1 );
	int video_win_num = theApp->cam_view->newVideoWindow( 
										new CStaticImageView( tmp ),
										new CFloatingPerspectiveView( tmp ),
										2 );

	if ( video_win_num == -1 )
	{
		CLOG( Msg::TOTAL_WINDOW_LIMIT_EXCEEDED, 0x0000ff );
		return;
	}

	
	theApp->cam_view->OnInitialUpdate();


	safe_state.Lock();
	{
		// add viewer node
		sprintf( tmp, "PERSPECTIVE#%d-viewer", num_perspectives+1 );
		node_pers_viewer[ num_perspectives ] = new PersViewer_Type
					( tmp, &MemPool, 
					  (CFloatingPerspectiveView*) theApp->cam_view->getVideoWindow( video_win_num ) );


		// add viewer icon node
		sprintf( tmp, "PERSPECTIVE#%d-icon_viewer", num_perspectives+1 );
		node_pers_icon[ num_perspectives ] = new PersIconViewer_Type
					( tmp, &MemPool, 
					  theApp->cam_view->getVideoWindowIcon( video_win_num ) );
		node_pers_icon[ num_perspectives ]->changeRate( icon_update_rate );


		// add dewarper node
		sprintf( tmp, "PERSPECTIVE_DEWARPER#%d-viewer", num_perspectives+1 );
		node_pers_dewarper[ num_perspectives ] = new PerspectiveDewarper_Type
					( tmp, &MemPool );


		// link video window's controls to dewarper node
		( (CFloatingPerspectiveView*) theApp->cam_view->getVideoWindow( video_win_num ) )->setNodeToken( num_perspectives );


		// init dewarper
		node_pers_dewarper[ num_perspectives ]->initMapping( 
										 params->getPerspectiveViewSizeX(), 
										 params->getPerspectiveViewSizeY(),
										 params->getMirrorCentreX(), params->getMirrorCentreY(),
										 params->getMirrorFocalLen(),
										 params->isCameraInverted(),
										 params->getMirrorVfov(),
										 true,
										 1, 30 * num_perspectives, 0, -10 );

		// register the threads
		registerThreadName( node_pers_viewer[ num_perspectives ]->threadID(),	
							node_pers_viewer[ num_perspectives ]->my_name );
		registerThreadName( node_pers_dewarper[ num_perspectives ]->threadID(),	
							node_pers_dewarper[ num_perspectives ]->my_name );


		// add to pipeline
		node_pers_dewarper[ num_perspectives ]->addSink( node_pers_icon[ num_perspectives ] );
		node_pers_dewarper[ num_perspectives ]->addSink( node_pers_viewer[ num_perspectives ] );

		if ( node_pal )
			node_pal->addSink( node_pers_dewarper[ num_perspectives ] );
		else
			node_reader->addSink( node_pers_dewarper[ num_perspectives ] );
		
		node_cameraController->addSink( node_pers_dewarper[ num_perspectives ] );


		// start the nodes
		node_pers_viewer[ num_perspectives ]->resume();
		node_pers_dewarper[ num_perspectives ]->resume();
		node_pers_icon[ num_perspectives ]->resume();

		num_perspectives++;
	}
	safe_state.Unlock();
}



