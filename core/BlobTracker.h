
#ifndef BLOB_TRACKER_H
#define BLOB_TRACKER_H


#pragma warning( disable : 4786 )
 
#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"
#include "BkgSubtracter.h"
#include "ObjectsInformation.h"
#include "MatchScores.h"
#include "Tracker.h"



#define USE_MASK


// these are only used for debug purposes...
//#define DUMP_CLUSTER_HISTOGRAM
//#define DUMP_OUTPUT			
//#define DUMP_OBJECTS






void calcClusterScoring( ObjectList* clusters, ObjectList* objects, 
						 MatchScores* match_scores );

void matchClusters2Objects(  ObjectList* clusters, 
							 ObjectList* objects,
							 ClusterObjectGraph* cluster_objects,
							 MatchScores* match_scores,
							 long frame );

void prepareLabelMap( IplImage* img, 
					  int foreground_value, 
					  IplImage* lbl_map,
					  int unlabeled_value,
					  IplImage* mask = 0 );

void removeSmallBlobs( BlobList* blobs, 
					   int minimum_area );

void groupNearbyBlobs( BlobList* blobs,
					   BlobClusterGraph* blob_clusters,
					   ObjectList* clusters,
					   IplImage* lbl_map,
					   int minimum_spatial_distance,
					   int minimum_radial_distance );

void gatherObjectInfo( BlobList* blobs, 
					   BlobClusterGraph* blob_clusters,
					   ObjectList* clusters,
					   IplImage* lbl_map,
					   IplImage* src_hsv,
					   int frame_nr );

void removeGhosts(	BlobList* blobs,
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects, 
					IplImage* lbl_map,
					IplImage* bkg,
					IplImage* bkg_hsv_mean,
					IplImage* src,
					IplImage* src_hsv,
					long frame, IplImage* out );

void dumpObjects(	BlobList* blobs,
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects, 
					IplImage* lbl_map,
					IplImage* src,
					long frame );

void drawOutput( IplImage* out, 
				 BlobList* blobs, 
				 BlobClusterGraph* blob_clusters,
				 ObjectList* clusters,
				 ClusterObjectGraph* cluster_objects,
				 ObjectList* objects,
				 IplImage* lbl_map,
				 IplImage* src,
				 long frame );





template< class ALLOCATOR >
class BlobTracker
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;


	typedef BkgSubtracter< ALLOCATOR >		BkgSubtr;


	IplImage* lbl_map;


	IplData out1;



	float obj_state_update_rate;
	float obj_state_update_rate_inv;


	// initial capacity for the lists (same value is used for the re-grow parameter)
	enum{	OBJECTS_INIT_LIST_CAPACITY	= 30	};
	enum{	CLUSTERS_INIT_LIST_CAPACITY	= 50	};
	enum{	BLOBS_INIT_LIST_CAPACITY	= 50	};



public:



	BlobTracker( string my_name, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		lbl_map = 0;
		out1.img = 0;


		::objects = new ObjectList( OBJECTS_INIT_LIST_CAPACITY, OBJECTS_INIT_LIST_CAPACITY );
		assert( objects );


		// parameters
		obj_state_update_rate = CfgParameters::instance()->getObjectStateUpdateRate();
		obj_state_update_rate_inv = 1.0 - obj_state_update_rate;
	}



	~BlobTracker()
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



			//------- get list of blobs from current frame
			iplSetFP( lbl_map, 0 );
			BlobList blobs( BLOBS_INIT_LIST_CAPACITY, BLOBS_INIT_LIST_CAPACITY );

			const int UNLABELED = 99999;
			assert( UNLABELED > blobs.capacity() );

			prepareLabelMap( bkg_res, BkgSubtr::FOREGROUND_PIXEL, 
							 lbl_map, UNLABELED, mask );
			labelBlobs( lbl_map, UNLABELED, &blobs, mask, 0 );

			removeSmallBlobs( &blobs, cfg->getMinBlobArea() );



			//-------- group blobs into blob clusters
			ObjectList clusters( CLUSTERS_INIT_LIST_CAPACITY, CLUSTERS_INIT_LIST_CAPACITY );

			BlobClusterGraph blob_clusters( &blobs, &blobs );

			groupNearbyBlobs( &blobs,
							  &blob_clusters,
							  &clusters,
							  lbl_map,
							  cfg->getMinBlobSpatialDistance(),
							  cfg->getMinBlobRadialDistance() );


			gatherObjectInfo( &blobs, 
							  &blob_clusters,
							  &clusters,
							  lbl_map,
							  src_hsv,
							  out1.frame );



			// does the blob cluster have a large enough radial size?
			if ( cfg->getMinBlobRadialSize() > 0 )
			{
				for ( int c = clusters.first(); c != -1; c = clusters.next( c ) )
				{
					Object* cls = clusters.getAt( c );
					assert( cls );

					if ( cls->max_radius - cls->min_radius < cfg->getMinBlobRadialSize() )
						clusters.remove( c );
				}
			}

			//--------- link objects to blob clusters
			ClusterObjectGraph cluster_objects( &clusters, objects );
			MatchScores match_scores( objects->capacity(), clusters.capacity() );

			calcClusterScoring( &clusters, objects, &match_scores );



			// objects match to the best cluster(s)
			matchClusters2Objects(  &clusters, objects, &cluster_objects, 
									&match_scores, out1.frame );



			// do some consistency checks across objects
			for ( int o = objects->first(); o != -1; o = objects->next( o ) )
			{
				Object* obj = objects->getAt( o );
				obj->occluded = false;

				// if an object has matched a cluster and this cluster is also
				// matched to another object, i.e. we have merged objects,
				// then we check that this object really overlaps the cluster.
				// Else we might have two objects marked as 'merged objects',
				// when in reality they are not. Such cases usually occur when
				// an object loses its cluster and then matches with the nearest
				// cluster it finds. But this cluster is matched to some other object.
				// So the object artifically 'merges' with the other object, causing
				// the real object to be frozen and causes itself to live longer if
				// its cluster has been lost.
				// This unless the object was really merged in previous frames. In
				// which case, because of the freezing of the state of the object,
				// the cluster might move outside the object's bounding box. In which
				// case we set its state to 'occluded'.
				int c = cluster_objects.firstLinkB( o );
				if ( c != -1 )
				{
					if ( cluster_objects.numLinksA( c ) > 1 )
					{
						if ( match_scores.overlap[ c ][ o ] == 0 )
						{
							if ( ! obj->was_last_merged )
							{
								cluster_objects.unlink( c, o );
							}
							else
							{
								obj->occluded = true;
							}
						}
					}
				}
			}

			
			// update object's visibility state
			for ( int o = objects->first(); o != -1; o = objects->next( o ) )
			{
				Object* obj = objects->getAt( o );


				// reset previous state
				obj->merged = false;
				obj->lost = false;


				// does object has a cluster linked to it?  (lost object)
				if ( cluster_objects.numLinksB( o ) > 0 )
				{
					++obj->lives;
					obj->lost = false;
					obj->lost_count = 0;
				}
				else
				{
					--obj->lives;
					obj->lost = true;
					++obj->lost_count;
				}


				if ( ! obj->lost )
				{
					// another object linked to the same cluster?  (merged objects and/or occlusion)
					int c = cluster_objects.firstLinkB( o );
					if ( cluster_objects.numLinksA( c ) > 1 )
					{
						obj->merged = true;
						obj->was_last_merged = true;
					}

					
					if ( ! obj->merged )
					{
						// multiple clusters linked to this object
						if ( cluster_objects.numLinksB( o ) > 1 )
						{
							obj->was_last_merged = false;
						}
					}
				}



				if ( obj->lost )
				{
					// delete an object if it has no lives left, i.e. has 
					// been lost for quite a while
					if ( obj->lives < 0 )
					{
						objects->remove( o );
					}
					// delete an object if it has got really small in size
					// and at some point it was much larger, i.e. it is not
					// just appearing (coming into view), and it has been
					// lost for some time
					else if ( obj->area < obj->max_area )
					{
						if ( obj->max_area > 2 * cfg->getMinBlobArea() &&
							 obj->max_area > 3 * obj->area &&
							 obj->lost_count > 10 )
						{
							objects->remove( o );
						}
					}
				}
			}


			// update object's state
			for ( int o = objects->first(); o != -1; o = objects->next( o ) )
			{
				Object* obj = objects->getAt( o );

				if ( ! obj->lost && ! obj->occluded && ! obj->merged )
				{
					int c = cluster_objects.firstLinkB( o );
					assert( cluster_objects.nextLinkB( o, c ) == -1 );

					Object* cls = clusters.getAt( c );

					// update object's state
					obj->area			= cls->area;
					obj->bounding_box	= cls->bounding_box;
					obj->centre			= cls->centre;
					obj->min_radius		= cls->min_radius;
					obj->max_radius		= cls->max_radius;
					obj->min_azimuth	= cls->min_azimuth;
					obj->max_azimuth	= cls->max_azimuth;

					if ( obj->area > obj->max_area )
						obj->max_area = obj->area;

					if ( obj->histogram_empty )
					{
						for ( int k = 0; k < Object::H_NUM_BINS; ++k )
							obj->h_bins[ k ] = cls->h_bins[ k ];
						for ( int k = 0; k < Object::S_NUM_BINS; ++k )
							obj->s_bins[ k ] = cls->s_bins[ k ];
						for ( int k = 0; k < Object::V_NUM_BINS; ++k )
							obj->v_bins[ k ] = cls->v_bins[ k ];
					}
					else
					{
						for ( int k = 0; k < Object::H_NUM_BINS; ++k )
							obj->h_bins[ k ] = cls->h_bins[ k ] * obj_state_update_rate + 
											   obj->h_bins[ k ] * obj_state_update_rate_inv;
						for ( int k = 0; k < Object::S_NUM_BINS; ++k )
							obj->s_bins[ k ] = cls->s_bins[ k ] * obj_state_update_rate + 
											   obj->s_bins[ k ] * obj_state_update_rate_inv;
						for ( int k = 0; k < Object::V_NUM_BINS; ++k )
							obj->v_bins[ k ] = cls->v_bins[ k ] * obj_state_update_rate + 
											   obj->v_bins[ k ] * obj_state_update_rate_inv;
					}
					obj->histogram_empty = false;
		


					// update object's history
					if ( obj->path == 0 )
					{
						obj->initHistory();
						obj->path->add( obj->centre );
					}
					else
					{
						// did we move enough to worth noticing it?
						CvPoint2D32f last_pos = *obj->path->getAt( obj->path->size()-1 );
						double d = sqrt(  SQR( last_pos.x - obj->centre.x ) +
										  SQR( last_pos.y - obj->centre.y ) );
						if ( d > 10 )
							obj->path->add( obj->centre );
					}
				}
			}


			// if there are any unmatched blob clusters then make them possible objects
			for ( int c = clusters.first(); c != -1; c = clusters.next( c ) )
			{
				if ( cluster_objects.numLinksA( c ) == 0 )		// cluster did not match with any object
				{
					Object* cls = clusters.getAt( c );

					int ndx = objects->getNextFreeIndex();

					cls->label = ndx;
					cls->lives = 5;		// initial number of lives

					objects->add( ndx, *cls );
				}
			}
 


			// eliminate ghosts
			if ( cfg->getRemoveGhosts() )
			{
				removeGhosts( &blobs, &blob_clusters, &clusters,
							  &cluster_objects, objects,
							  lbl_map, bkg_rgb_mean, bkg_hsv_mean, 
							  src, src_hsv, out1.frame, out1.img );
			}


			SetEvent( BkgSubtr::ready_signal );



			// prepare something to display...
			drawOutput( out1.img, &blobs, &blob_clusters, &clusters, &cluster_objects,
						objects, lbl_map, src, out1.frame );


#ifdef DUMP_OBJECTS
			dumpObjects( &blobs, &blob_clusters, &clusters, &cluster_objects, objects, 
						 lbl_map, src, out1.frame );
#endif

#ifdef DUMP_OUTPUT			
			{
				char tmp[200];
				sprintf( tmp, "\\out\\%04.04d_output.bmp", out1.frame );
				cvSaveImage( tmp, out1.img );
			}
#endif


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
