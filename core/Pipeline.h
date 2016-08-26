
#ifndef PIPELINE_H
#define PIPELINE_H


#include "log.h"
#include "..\tools\singleton.h"
#include "..\framework\framework.h"
#include "..\gui\StaticImageView.h"
#include "..\gui\MainFrm.h"
#include "SyncNodeWithStatistics.h"
#include "PanoramicDewarper.h"
#include "PerspectiveDewarper.h"
#include "BkgSubtracter.h"
#include "IterativeThreshold.h"
#include "EdgeDetector.h"
#include "BoundaryDetector.h"
#include "BlobTracker.h"
#include "ColourTracker.h"
#include "VirtualCameraController.h"
#include "HistoryNode.h"


class CFloatingImageView;
class CFloatingOmniImageView;
class CFloatingPerspectiveView;
class CFloatingPanoramaView;



#define MAX_PERSPECTIVES	10
#define MAX_PANORAMAS		3



class Pipeline : public RunnableObject, public Singleton< Pipeline >
{
public:

	typedef MemoryPool< IplImageResource >		memory_pool_type;
	
	memory_pool_type MemPool;


	typedef ReadJpegSequence< memory_pool_type >						ReadJpegSequence_Type;
	typedef CorrectAspectRatio< memory_pool_type >						CorrectAspectRatio_Type;
	typedef SyncNodeWithStatistics< IplData, IplData, CMainFrame >		Sync_Type; 
	typedef ViewerNode< memory_pool_type, CStaticImageView >			OmniIconViewer_Type;
	typedef ViewerNode< memory_pool_type, CFloatingOmniImageView >		OmniViewer_Type;
	typedef PanoramicDewarper< memory_pool_type >						PanoramicDewarper_Type;
	typedef ViewerNode< memory_pool_type, CStaticImageView >			PanoIconViewer_Type; 
	typedef ViewerNode< memory_pool_type, CFloatingPanoramaView >		PanoViewer_Type; 
	typedef PerspectiveDewarper< memory_pool_type >						PerspectiveDewarper_Type;
	typedef ViewerNode< memory_pool_type, CStaticImageView >			PersIconViewer_Type; 
	typedef ViewerNode< memory_pool_type, CFloatingPerspectiveView >	PersViewer_Type; 
	typedef Colour2GrayNode< memory_pool_type >							ToGray_Type; 
	typedef BkgSubtracter< memory_pool_type >							BkgSubtracter_Type;
	typedef BlobTracker< memory_pool_type >								BlobTracker_Type;
	typedef ColourTracker< memory_pool_type >							ColourTracker_Type;
	typedef HistoryNode< memory_pool_type >								HistoryNode_Type;
	typedef VirtualCameraController< memory_pool_type >					CameraController_Type;
	typedef IterativeThreshold< memory_pool_type >						Threshold_Type;
	typedef EdgeDetector< memory_pool_type >							EdgeDetector_Type;
	typedef BoundaryDetector< memory_pool_type >						BoundaryDetector_Type;
	typedef ViewerNode< memory_pool_type, CStaticImageView >			BkgIconViewer_Type; 
	typedef ViewerNode< memory_pool_type, CFloatingImageView >			BkgSubtractViewer_Type;
	typedef SlideshowViewerNode< memory_pool_type, CFloatingOmniImageView, 400 >	SlideshowViewer_Type;


	CCriticalSection safe_state;		// safe to get pointer to nodes

	Timer clock;
	double total_time;					// used to keep track of pauses


private:

	ReadJpegSequence_Type*		node_reader;
	CorrectAspectRatio_Type*	node_pal;
	OmniIconViewer_Type*		node_iconvw; 
	OmniViewer_Type*			node_viewer; 
	
	PanoramicDewarper_Type*		node_pano_dewarper[ MAX_PANORAMAS ];
	PanoViewer_Type*			node_pano_viewer[ MAX_PANORAMAS ]; 
	PanoIconViewer_Type*		node_pano_icon[ MAX_PANORAMAS ]; 

	PerspectiveDewarper_Type*	node_pers_dewarper[ MAX_PERSPECTIVES ];
	PersViewer_Type*			node_pers_viewer[ MAX_PERSPECTIVES ]; 
	PersIconViewer_Type*		node_pers_icon[ MAX_PERSPECTIVES ]; 
	
	BkgSubtractViewer_Type*		node_viewer3; 
	BkgSubtractViewer_Type*		node_viewer4; 
	BkgSubtractViewer_Type*		node_viewer5; 
	ToGray_Type*				node_gray;
	Sync_Type*					node_sync;
	BkgSubtracter_Type*			node_bkgSubtracter;
	BlobTracker_Type*			node_blobTracker;
	ColourTracker_Type*			node_colourTracker;
	CameraController_Type*		node_cameraController;
	HistoryNode_Type*			node_history;
	BkgIconViewer_Type*			node_iconvw3;
	BkgIconViewer_Type*			node_iconvw4;
	BkgIconViewer_Type*			node_iconvw5;

	Threshold_Type*				node_threshold;
	EdgeDetector_Type*			node_edge;
	BoundaryDetector_Type*		node_boundary;
	SlideshowViewer_Type*		node_slideshow;


	int num_perspectives;
	int num_panoramas;

	int icon_update_rate;


	HANDLE	do_stop;
	bool	is_suspended;
	bool	is_initialising;



public:

	virtual UINT loop();


	virtual DWORD resume();


	virtual DWORD suspend();


	inline bool isSuspended()
	{
		return is_suspended;
	}



	bool isInitialising()
	{
		return is_initialising;
	}



	DWORD end( DWORD block_time = 0 )
	{
		if ( is_suspended )
			resume();
		SetEvent( do_stop );
		return wait( block_time );
	}



	inline PanoramicDewarper_Type* getPanoramicNode( int token )
	{
		if ( token < 0 && token >= num_panoramas )
			return 0;

		return node_pano_dewarper[ token ];
	}



	inline PerspectiveDewarper_Type* getPerspectiveNode( int token )
	{
		if ( token < 0 && token >= num_perspectives )
			return 0;

		return node_pers_dewarper[ token ];
	}


	
	inline int getNumPerspectives()
	{
		return num_perspectives;
	}



	CameraController_Type* getVirtualCameraController()
	{
		return node_cameraController;
	}


	void OnFolderButton2();
	void OnNewPerspectiveView();



	void doCalibration();


protected:

	// Pipeline cannot be constructed directly because it is a 
	// Singleton class - the singleton must be obtained through
	// calling static member function Instance()

	Pipeline();


	virtual ~Pipeline();



	// since we made ctor and dtor protected, we must declare the
	// following template classes as friends.  Singleton<A> will need
	// to call the constructor to create the one and only instance;
	// while SingletonDestroyer<A> will need to call the destructor
	// when destroying the one and only instance.
	friend class Singleton< Pipeline >;
	friend class SingletonDestroyer< Pipeline >;

	
	void initNodes();


	void killNodes();


	void initGUI();

};



#endif
