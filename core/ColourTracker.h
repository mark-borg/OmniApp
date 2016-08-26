
#ifndef COLOUR_TRACKER_H
#define COLOUR_TRACKER_H


#pragma warning( disable : 4786 )
 
#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"
#include "BkgSubtracter.h"
#include "ObjectsInformation.h"
#include "Tracker.h"
#include "BivariateGaussian.h"
#include "MixtureModel.h"
#include "EM.h"
 


#define USE_MASK





void updateObjectGMM( 	ObjectList* objects,
						IplImage* src,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl );

void fitColourGMM( 	BlobList* blobs, 
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects,
					int obj_id,
					IplImage* lbl_map,
					IplImage* src,
					IplImage* src_hsv,
					long frame );


void trackColourGMM(	ObjectList* objects,
						IplImage* bkg_res,
						int foreground,
						IplImage* src,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl );

void removeCloseBlobs(	BlobList* blobs, 
						IplImage* lbl_map,
						IplImage* src_hsv,
						long frame );


void drawOutput2( IplImage* out, 
				 BlobList* blobs, 
				 BlobClusterGraph* blob_clusters,
				 ObjectList* clusters,
				 ClusterObjectGraph* cluster_objects,
				 ObjectList* objects,
				 IplImage* lbl_map,
				 IplImage* src,
				 long frame );





template< class ALLOCATOR >
class ColourTracker
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;


	typedef BkgSubtracter< ALLOCATOR >		BkgSubtr;


	IplImage* lbl_map;


	IplData out1;



	// initial capacity for the lists (same value is used for the re-grow parameter)
	enum{	OBJECTS_INIT_LIST_CAPACITY	= 30	};
	enum{	CLUSTERS_INIT_LIST_CAPACITY	= 50	};
	enum{	BLOBS_INIT_LIST_CAPACITY	= 50	};



public:



	ColourTracker( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		lbl_map = 0;
		out1.img = 0;


		::objects = new ObjectList( OBJECTS_INIT_LIST_CAPACITY, OBJECTS_INIT_LIST_CAPACITY );
		assert( objects );
	}



	~ColourTracker()
	{
		if ( lbl_map != 0 )
			allocator->release( lbl_map );
		if ( out1.img != 0 )
			allocator->release( out1.img );

		delete ::objects;
	}
	


	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		assert( source_buffers.size() == 5 );

		IplImage* bkg_res = source_buffers[ 0 ].img;
		assert( bkg_res );
		assert( bkg_res->nChannels == 1 );
		assert( bkg_res->depth == IPL_DEPTH_8U );

		IplImage* bkg_rgb_mean = source_buffers[ 1 ].img;
		assert( bkg_rgb_mean );
		assert( bkg_rgb_mean->nChannels == 3 );
		assert( bkg_rgb_mean->depth == IPL_DEPTH_32S );

		IplImage* bkg_hsv_mean = source_buffers[ 2 ].img;
		assert( bkg_hsv_mean );
		assert( bkg_hsv_mean->nChannels == 3 );
		assert( bkg_hsv_mean->depth == IPL_DEPTH_32S );

		IplImage* src = source_buffers[ 3 ].img;
		assert( src );
		assert( src->nChannels == 3 );
		assert( src->depth == IPL_DEPTH_8U );

		IplImage* src_hsv = source_buffers[ 4 ].img;
		assert( src_hsv );
		assert( src_hsv->nChannels == 3 );
		assert( src_hsv->depth == IPL_DEPTH_32S );
		

		CfgParameters* cfg = CfgParameters::instance();


#ifdef USE_MASK
		IplImage* mask = cfg->getMask();
#else
		IplImage* mask = 0;
#endif


		if ( lbl_map == 0 )				// create buffers
		{
			IplImageResource typ = IplImageResource::getType( src );
			typ.channels = 1;
			typ.depth = IPL_DEPTH_32F;

			lbl_map = allocator->acquireClean( typ );
			assert( lbl_map );
		}


		
		if ( ! source_buffers[ 0 ].ignore )
		{
			IplImageResource typ = IplImageResource::getType( src );

			assert( out1.img == 0 );
			out1.img = allocator->acquireClean( typ );
			assert( out1.img );
			out1.frame = source_buffers[ 0 ].frame;



			//---------- colour tracking algorithm
			const int UNLABELED = 99999;

			iplSetFP( lbl_map, 0 );
			

			// track existing objects by colour
			trackColourGMM( objects, bkg_res, BkgSubtr::FOREGROUND_PIXEL, 
							src, src_hsv, out1.frame, lbl_map );


			
			// find new objects
			BlobList blobs( BLOBS_INIT_LIST_CAPACITY, BLOBS_INIT_LIST_CAPACITY );
			ObjectList clusters( CLUSTERS_INIT_LIST_CAPACITY, CLUSTERS_INIT_LIST_CAPACITY );
			BlobClusterGraph blob_clusters( &blobs, &blobs );
			ClusterObjectGraph cluster_objects( &clusters, objects );

			labelBlobs( lbl_map, UNLABELED, &blobs, mask, cfg->getMinColourArea() );

			// are they really new objects, or parts of existing objects?
			removeCloseBlobs( &blobs, lbl_map, src_hsv, out1.frame );

			
			// take care of fragmented objects
			groupNearbyBlobs( &blobs,
							  &blob_clusters,
							  &clusters,
							  lbl_map, 
							  cfg->getMinColourSpatialDistance(),
							  cfg->getMinColourRadialDistance() );

			
			gatherObjectInfo( &blobs, 
							  &blob_clusters,
							  &clusters,
							  lbl_map, 
							  src_hsv,
							  out1.frame );


			// does the blob cluster have a large enough radial size?
			if ( cfg->getMinColourRadialSize() > 0 )
			{
				for ( int c = clusters.first(); c != -1; c = clusters.next( c ) )
				{
					Object* cls = clusters.getAt( c );
					assert( cls );

					if ( cls->max_radius - cls->min_radius < cfg->getMinColourRadialSize() )
						clusters.remove( c );
				}
			}
			
			for ( int c = clusters.first(); c != -1; c = clusters.next( c ) )
			{
				// cluster did not match with any existing object
				if ( cluster_objects.numLinksA( c ) == 0 )
				{
					// create new object and add to list of objects
					Object* cls = clusters.getAt( c );

					int obj_id = objects->getNextFreeIndex();

					cls->label = obj_id;
					cls->lives = 5;		// initial number of lives
					cls->first_pos = cls->centre;

					objects->add( obj_id, *cls );
					cluster_objects.link( c, obj_id );

					
					// now fit a gaussian mixture model to this new object
					fitColourGMM( &blobs, &blob_clusters, &clusters, &cluster_objects,
								  objects, obj_id, lbl_map, src, src_hsv, out1.frame );
				}
			}

			updateObjectGMM( objects, src, src_hsv, out1.frame, lbl_map );
			
			//---------


			SetEvent( BkgSubtr::ready_signal );


			// prepare something to display...
			drawOutput2( out1.img, &blobs, &blob_clusters, &clusters, &cluster_objects,
						 objects, lbl_map, src, out1.frame );


			// move result to sink buffers
			for ( int i = 0; i < sink_buffers.size(); ++i )
				sink_buffers[ i ] = out1;
		}
		else
		{
			SetEvent( BkgSubtr::ready_signal );


			IplImageResource typ = IplImageResource::getType( bkg_res );
			typ.channels = 3;

			assert( out1.img == 0 );
			out1.img = allocator->acquire( typ );
			assert( out1.img );


			strncpy( bkg_res->colorModel, "Gray", 4 );
			iplGrayToColor( bkg_res, out1.img, 1, 1, 1 );
			
			
			// push image to sink node(s)
			for ( int i = 0; i < sink_buffers.size(); ++i )
				sink_buffers[ i ] = out1;
		}

	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( out1.img != 0 )
		{
			allocator->release( out1.img );
			out1.img = 0;
		}
	}

};



#endif
