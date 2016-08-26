
#include <math.h>
#include "BlobTracker.h"
#include "..\gui\stdafx.h"
#include "..\tools\timer.h"
#include "..\tools\trig.h"
#include "CfgParameters.h"
#include <highgui.h>
#include "Pipeline.h"



// this is only used for debug purposes...
//#define DUMP_MATCH_SCORE_TABLE




void prepareLabelMap( IplImage* img, 
					  int foreground_value, 
					  IplImage* lbl_map,
					  int unlabeled_value,
					  IplImage* mask )
{
	assert( img->depth == IPL_DEPTH_8U );
	assert( img->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( lbl_map->nChannels == 1 );

	bool using_mask = ( mask != 0 );


	if ( using_mask )
	{
		for ( int y = 0; y < img->height; y++ )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_img, img, y );
			COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_mask, mask, y );

			for ( int x = 0; x < img->width; ++x )
			{
				if ( p_mask[ x ] )
				{
					if ( p_img[ x ] == foreground_value )
						p_lbl_map[ x ] = unlabeled_value;
				}
			}
		}
	}
	else
	{
		for ( int y = 0; y < img->height; y++ )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_img, img, y );
			COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );

			for ( int x = 0; x < img->width; ++x )
			{
				if ( p_img[ x ] == foreground_value )
					p_lbl_map[ x ] = unlabeled_value;
			}
		}
	}
}




void removeSmallBlobs( BlobList* blobs, 
					   int minimum_area )
{
	assert( blobs );

	for ( int b = blobs->first(); b != -1; b = blobs->next( b ) )
	{
		Blob* blob = blobs->getAt( b );
		if ( blob->area < minimum_area )
			blobs->remove( b );
	}
}




double radialAngle( Blob* blob1, Blob* blob2 )
{
	CfgParameters* cfg = CfgParameters::instance();

	int cx = cfg->getMirrorCentreX();
	int cy = cfg->getMirrorCentreY();


	float** p_azimuth_map = cfg->getAzimuthMapPtrs();
	int width = cfg->getAzimuthMap()->width;
	int height = cfg->getAzimuthMap()->height;


	double b1ang, b2ang;


	// blob 1
	{
		int x1 = blob1->bounding_box.x;
		int y1 = blob1->bounding_box.y;
		double a1 = p_azimuth_map[ y1 ][ x1 ];

		int x2 = blob1->bounding_box.x + blob1->bounding_box.width;
		int y2 = blob1->bounding_box.y;
		// position may fall outside LUT map
		double a2 = ( x2 < width && y2 < height ) ? p_azimuth_map[ y2 ][ x2 ] : cfg->calcAzimuth( x2, y2 );

		int x3 = blob1->bounding_box.x + blob1->bounding_box.width;
		int y3 = blob1->bounding_box.y + blob1->bounding_box.height;
		// position may fall outside LUT map
		double a3 = ( x3 < width && y3 < height ) ? p_azimuth_map[ y3 ][ x3 ] : cfg->calcAzimuth( x3, y3 );

		int x4 = blob1->bounding_box.x;
		int y4 = blob1->bounding_box.y + blob1->bounding_box.height;
		// position may fall outside LUT map
		double a4 = ( x4 < width && y4 < height ) ? p_azimuth_map[ y4 ][ x4 ] : cfg->calcAzimuth( x4, y4 );

		b1ang = ( a1 + a2 + a3 + a4 ) / 4.0;
	}


	// blob 2
	{
		int x1 = blob2->bounding_box.x;
		int y1 = blob2->bounding_box.y;
		double a1 = p_azimuth_map[ y1 ][ x1 ];

		int x2 = blob2->bounding_box.x + blob1->bounding_box.width;
		int y2 = blob2->bounding_box.y;
		// position may fall outside LUT map
		double a2 = ( x2 < width && y2 < height ) ? p_azimuth_map[ y2 ][ x2 ] : cfg->calcAzimuth( x2, y2 );

		int x3 = blob2->bounding_box.x + blob1->bounding_box.width;
		int y3 = blob2->bounding_box.y + blob1->bounding_box.height;
		// position may fall outside LUT map
		double a3 = ( x3 < width && y3 < height ) ? p_azimuth_map[ y3 ][ x3 ] : cfg->calcAzimuth( x3, y3 );

		int x4 = blob2->bounding_box.x;
		int y4 = blob2->bounding_box.y + blob1->bounding_box.height;
		// position may fall outside LUT map
		double a4 = ( x4 < width && y4 < height ) ? p_azimuth_map[ y4 ][ x4 ] : cfg->calcAzimuth( x4, y4 );

		b2ang = ( a1 + a2 + a3 + a4 ) / 4.0;
	}


	double d = fabs( b1ang - b2ang );
	if ( d > 180.0 )
		d = 360.0 - d;

	return d;
}




void groupNearbyBlobs( BlobList* blobs,
					   BlobClusterGraph* blob_clusters,
					   ObjectList* clusters,
					   IplImage* lbl_map,
					   int minimum_spatial_distance,
					   int minimum_radial_distance )	//[0..180]; -1 to disable radial checking
{
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( lbl_map->nChannels == 1 );
	assert( blobs );
	assert( clusters );
	assert( blob_clusters );
	
		
	// initially each blob belongs to a separate blob cluster
	blob_clusters->selfLink();


	bool check_radial = minimum_radial_distance > 0;
	if ( minimum_radial_distance > 180 )
		minimum_radial_distance = 180;


	// group blobs that are spatially or radially near each other
	for ( int b1 = blobs->first(); b1 != -1; b1 = blobs->next( b1 ) )
	{
		Blob* blob1 = blobs->getAt( b1 );

		for ( int b2 = blobs->next( b1 ); b2 != -1; b2 = blobs->next( b2 ) )
		{
			Blob* blob2 = blobs->getAt( b2 );

			int group_id1 = blob_clusters->firstLinkA( b1 );
			int group_id2 = blob_clusters->firstLinkA( b2 );

			if ( group_id1 != group_id2 )
			{
				double min_distance;
				CvPoint min_pixel;
				nearestDistance( lbl_map, 
								 blob1->label, blob2->label, 
								 blob1->bounding_box, blob2->bounding_box,
								 minimum_spatial_distance,
								 min_distance, min_pixel );

				double min_radial = 360;
				if ( check_radial )
					min_radial = radialAngle( blob1, blob2 );

				if ( min_distance < minimum_spatial_distance ||
					 min_radial < minimum_radial_distance )
				{
					// change cluster 'id' of blob 2 to be same as that of blob 1
					blob_clusters->unlink( b2, group_id2 );
					blob_clusters->link( b2, group_id1 );

					// change any other blob which had cluster id of blob 2 to now
					// belong to cluster of blob 1
					for ( int g = blob_clusters->firstLinkB( group_id2 ); g != -1; g = blob_clusters->nextLinkB( group_id2, g ) )
					{
						blob_clusters->unlink( g, group_id2 );
						blob_clusters->link( g, group_id1 );
					}
				}
			}
		}
	}


	// now build list of blob clusters
	assert( clusters->size() == 0 );

	for ( int b = blobs->first(); b != -1; b = blobs->next( b ) )
	{
		if ( blob_clusters->firstLinkB( b ) != -1 )
		{
			Object cls;
			cls.label = b;

			int i = clusters->add( b, cls );
			assert( i == b );
		}
	}
}




IplImage* makeHSVhistogram( Object* obj )
{
	const int xscale = 10;
	const int yscale = 200;

	int max_bins = MAX( MAX( Object::H_NUM_BINS, Object::S_NUM_BINS ), Object::V_NUM_BINS );
	int width = max_bins * xscale;
	int height = 1.0 * yscale;		// as histogram values are normalised to [0.0 .. 1.0]


	IplImage* histo = cvCreateImage( cvSize( width, height ), IPL_DEPTH_8U, 3 );
	iplSet( histo, 0 );
		
	float hscale = xscale * max_bins / Object::H_NUM_BINS;
	float sscale = xscale * max_bins / Object::S_NUM_BINS;
	float vscale = xscale * max_bins / Object::V_NUM_BINS;


	for ( int k = 1; k < Object::H_NUM_BINS; ++k )
	{
		cvLine( histo, 
				cvPoint( (k-1) * hscale, yscale - obj->h_bins[ k-1 ] * yscale -1 ),
				cvPoint( k * hscale, yscale - obj->h_bins[ k ] * yscale -1 ),
				CV_RGB( 0, 0, 255 ) );
		cvRectangle( histo,
					 cvPoint( (k-1) * hscale -1, yscale - obj->h_bins[ k-1 ] * yscale -2 ),
					 cvPoint( (k-1) * hscale +1, yscale - obj->h_bins[ k-1 ] * yscale ),
					 CV_RGB( 0, 0, 255 ) );
		cvRectangle( histo,
					 cvPoint( k * hscale -1, yscale - obj->h_bins[ k ] * yscale -2 ),
					 cvPoint( k * hscale +1, yscale - obj->h_bins[ k ] * yscale ),
					 CV_RGB( 0, 0, 255 ) );
	}
		
	for ( int k = 1; k < Object::S_NUM_BINS; ++k )
	{
		cvLine( histo, 
				cvPoint( (k-1) * sscale, yscale - obj->s_bins[ k-1 ] * yscale -1 ),
				cvPoint( k * sscale, yscale - obj->s_bins[ k ] * yscale -1 ),
				CV_RGB( 0, 255, 0 ) );
		cvRectangle( histo,
					 cvPoint( (k-1) * sscale -1, yscale - obj->s_bins[ k-1 ] * yscale -2 ),
					 cvPoint( (k-1) * sscale +1, yscale - obj->s_bins[ k-1 ] * yscale ),
					 CV_RGB( 0, 255, 0 ) );
		cvRectangle( histo,
					 cvPoint( k * sscale -1, yscale - obj->s_bins[ k ] * yscale -2 ),
					 cvPoint( k * sscale +1, yscale - obj->s_bins[ k ] * yscale ),
					 CV_RGB( 0, 255, 0 ) );
	}
		
	for ( int k = 1; k < Object::V_NUM_BINS; ++k )
	{
		cvLine( histo, 
				cvPoint( (k-1) * vscale, yscale - obj->v_bins[ k-1 ] * yscale -1 ),
				cvPoint( k * vscale, yscale - obj->v_bins[ k ] * yscale -1 ),
				CV_RGB( 255, 0, 0 ) );
		cvRectangle( histo,
					 cvPoint( (k-1) * vscale -1, yscale - obj->v_bins[ k-1 ] * yscale -2 ),
					 cvPoint( (k-1) * vscale +1, yscale - obj->v_bins[ k-1 ] * yscale ),
					 CV_RGB( 255, 0, 0 ) );
		cvRectangle( histo,
					 cvPoint( k * vscale -1, yscale - obj->v_bins[ k ] * yscale -2 ),
					 cvPoint( k * vscale +1, yscale - obj->v_bins[ k ] * yscale ),
					 CV_RGB( 255, 0, 0 ) );
	}

	return histo;
}




void gatherObjectInfo( BlobList* blobs, 
					   BlobClusterGraph* blob_clusters,
					   ObjectList* clusters,
					   IplImage* lbl_map,
					   IplImage* src_hsv,
					   int frame_nr )
{
	assert( blobs );
	assert( blob_clusters );
	assert( clusters );
	assert( lbl_map );
	assert( src_hsv );
	assert( lbl_map->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );

	
	int mcx = CfgParameters::instance()->getMirrorCentreX();
	int mcy = CfgParameters::instance()->getMirrorCentreY();

	float** p_azimuth_map = CfgParameters::instance()->getAzimuthMapPtrs();
	float** p_radial_map = CfgParameters::instance()->getRadialMapPtrs();

	
	const int LOW_HSV_THRESHOLD = 0.02 * 256;


	for ( int c = clusters->first(); c != -1; c = clusters->next( c ) )
	{
		Object* cls = clusters->getAt( c );

		long area = 0;
		double cx = 0.0, cy = 0.0;
		CvRect box = cvRect( -1, -1, 0, 0 );
		int min_radius = 999999, max_radius = -1;
		double min_azimuth = 9999, max_azimuth = -1;
		long count_hsv = 0;

		double prev_azimuth = -9999;
		int boundary_fix = 0;

		// for all the blobs of this blob cluster
		for ( int b = blob_clusters->firstLinkB( c ); b != -1; b = blob_clusters->nextLinkB( c, b ) )
		{
			Blob* blob = blobs->getAt( b );

			
			if ( box.width == 0 )
				box = blob->bounding_box;
			else
				box = unionRect( box, blob->bounding_box );


			int xmin = blob->bounding_box.x;
			int xmax = blob->bounding_box.x + blob->bounding_box.width;
			int ymin = blob->bounding_box.y;
			int ymax = blob->bounding_box.y + blob->bounding_box.height;

			for ( int y = ymin; y <= ymax; ++y )
			{
				COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );
				COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y );

				for ( int x = xmin, x3 = xmin*3; x <= xmax; ++x, x3 +=3 )
				{
					if ( p_lbl_map[ x ] == blob->label )
					{
						++area;


						cx += x;
						cy += y;


						// distance of pixel from mirror centre
						int alt = lrintf( p_radial_map[ y ][ x ] );

						if ( alt < min_radius )
							min_radius = alt;
						if ( alt > max_radius )
							max_radius = alt;

						// azimuth range
						double az = p_azimuth_map[ y ][ x ];

						// have we crossed the boundary?
						if ( prev_azimuth != -9999 && boundary_fix == 0 &&
							 ( ( prev_azimuth >= 359 && az <= 1 ) || ( prev_azimuth <= 1 && az >= 359 ) ) )
						{
							// going from 360-edge to 0-edge
							if ( prev_azimuth >= 359 )
								boundary_fix = 360; 
							// going from 0-edge to 360-edge
							else
								boundary_fix = -360;
						}

						// do we need to fix any boundary cross-overs?
						if ( boundary_fix == 360 && az < 180 )
							az += boundary_fix;
						if ( boundary_fix == -360 && az > 180 )
							az += boundary_fix;

						if ( az < min_azimuth )
							min_azimuth = az;
						if ( az > max_azimuth )
							max_azimuth = az;
						prev_azimuth = az;



						// HSV histogram
						int hi = p_src_hsv[ x3    ];
						int si = p_src_hsv[ x3 +1 ];
						int vi = p_src_hsv[ x3 +2 ];

						hi = hi >= 360 ? hi - 360 : hi;
						int hh = hi / ( 360.0 / Object::H_NUM_BINS );
						int ss = si / ( 256.0 / Object::S_NUM_BINS );
						int vv = vi / ( 256.0 / Object::V_NUM_BINS );

						assert( hh >= 0 && hh < Object::H_NUM_BINS );
						assert( ss >= 0 && ss < Object::S_NUM_BINS );
						assert( vv >= 0 && vv < Object::V_NUM_BINS );

						if ( vi > LOW_HSV_THRESHOLD && si > LOW_HSV_THRESHOLD )
						{
							++cls->h_bins[ hh ];
							++cls->s_bins[ ss ];
							++cls->v_bins[ vv ];
							++count_hsv;
						}
					}
				}
			}
		}

		cx /= area;
		cy /= area;


		for ( int k = 0; k < Object::H_NUM_BINS; ++k )
			cls->h_bins[ k ] /= count_hsv;
		for ( int k = 0; k < Object::S_NUM_BINS; ++k )
			cls->s_bins[ k ] /= count_hsv;
		for ( int k = 0; k < Object::V_NUM_BINS; ++k )
			cls->v_bins[ k ] /= count_hsv;


		cls->area			= area;
		cls->centre			= cvPoint2D32f( cx, cy );
		cls->first_pos		= cvPoint2D32f( cx, cy );
		cls->bounding_box	= box;
 		cls->born			= frame_nr;
		cls->lives			= 1;
		
		cls->min_radius		= min_radius;
		cls->max_radius		= max_radius;
		cls->min_azimuth	= min_azimuth;
		cls->max_azimuth	= max_azimuth;



#ifdef DUMP_CLUSTER_HISTOGRAM
		IplImage* histo = makeHSVhistogram( cls );
		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_cluster%02.02d_hsv_histo.bmp", frame_nr, c );
		cvSaveImage( tmp, histo );
		cvReleaseImage( &histo );


		// dump HS-slice (at V=255) of HSV space 
		static bool first_time = true;
		if ( first_time )
		{
			int width = 320;
			int height = 128;

			IplImage* tmp = cvCreateImage( cvSize( width, height ), IPL_DEPTH_8U, 3 );
			iplSet( tmp, 0 );

			for ( int h = 0; h < 360; ++h )
			{
				for ( int s = 5; s < 255; ++s )
				{
					unsigned char r, g, b;
					hsv2rgb( h, s, 255, r, g, b );
					cvLine( tmp, 
							cvPoint( h * width / 360.0, s/2 ), 
							cvPoint( h * width / 360.0, s/2+1 ), 
							CV_RGB( b, g, r ), 1 );
				}
			}

			cvSaveImage( "\\out\\hsv_hs_slice.bmp", tmp );
			cvReleaseImage( &tmp );
		}
		first_time = false;		
#endif

	}
}




double bhattacharyyaCoefficient( 
				   double h_bins1[], double h_bins2[],
				   double s_bins1[], double s_bins2[],
				   double v_bins1[], double v_bins2[] )
{
	double bc = 0;

	for ( int k = 0; k < Object::H_NUM_BINS; ++k )
		bc += sqrt( h_bins1[ k ] * h_bins2[ k ] );

	for ( int k = 0; k < Object::S_NUM_BINS; ++k )
		bc += sqrt( s_bins1[ k ] * s_bins2[ k ] );

	for ( int k = 0; k < Object::V_NUM_BINS; ++k )
		bc += sqrt( v_bins1[ k ] * v_bins2[ k ] );

	bc /= 3.0;
	if ( bc > 1.0 )
		bc = 1.0;

	return bc;
}




void calcClusterScoring( ObjectList* clusters, ObjectList* objects, 
						 MatchScores* match_scores )
{
	CfgParameters* cfg = CfgParameters::instance();

	
	double mirror_H = cfg->getMirrorFocalLen() * 2;
	double mirror_b = cfg->getMirrorBoundary();


	// objects compare themselves to the blob clusters of this frame
	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		for ( int c = clusters->first(); c != -1; c = clusters->next( c ) )
		{
			Object* cls = clusters->getAt( c );

					
			//---- bounding box overlap measure
#if USE_CARTESIAN_BOUNDING_BOX
			CvRect overlap_box = intersectRect( obj->bounding_box, cls->bounding_box );
			double overlap = 0;

			if ( overlap_box.width > 0 && overlap_box.height > 0 )
			{
				double overlap_area = overlap_box.width * overlap_box.height;
				double obj_area = obj->bounding_box.width * obj->bounding_box.height;
				double cls_area = cls->bounding_box.width * cls->bounding_box.height;

				overlap = ( 2.0 * overlap_area ) / ( obj_area + cls_area );
			}
#else
			CvRect obj_box = cvRect( obj->min_radius, obj->min_azimuth, 
									 lrintf( obj->max_radius - obj->min_radius ), 
									 lrintf( obj->max_azimuth - obj->min_azimuth ) );
			CvRect cls_box = cvRect( cls->min_radius, cls->min_azimuth, 
									 lrintf( cls->max_radius - cls->min_radius ), 
									 lrintf( cls->max_azimuth - cls->min_azimuth ) );

			// adjust in case of azimuth boundary cross-over
			if ( obj->max_azimuth > 360.0 && cls->max_azimuth < 180.0 ) 
				cls_box.y += 360;
			if ( cls->max_azimuth > 360.0 && obj->max_azimuth < 180.0 ) 
				obj_box.y += 360;
			if ( obj->max_azimuth < 0.0 && cls->max_azimuth > 180.0 ) 
				cls_box.y -= 360;
			if ( cls->max_azimuth < 0.0 && obj->max_azimuth > 180.0 ) 
				obj_box.y -= 360;
	
			// scale altitude and azimuth to arbitrary units to make them
			// equal (in scale) so that overlap ratio is meaningful
			float az_factor = mirror_H / 90.0;
			obj_box.y *= az_factor;
			obj_box.height *= az_factor;
			cls_box.y *= az_factor;
			cls_box.height *= az_factor;

			// calculate overlap area ratio
			CvRect overlap_box = intersectRect( obj_box, cls_box );
			double overlap = 0;

			if ( overlap_box.width > 0 && overlap_box.height > 0 )
			{
				double overlap_area = overlap_box.width * overlap_box.height;
				double obj_area = obj_box.width * obj_box.height;
				double cls_area = cls_box.width * cls_box.height;

				overlap = ( 2.0 * overlap_area ) / ( obj_area + cls_area );
			}
#endif
			assert( overlap >= 0 && overlap <= 1 );
			match_scores->overlap[ c ][ o ] = overlap;



			//---- centroid distance measure
			double distance = sqrt(	SQR( cls->centre.x - obj->centre.x ) +
									SQR( cls->centre.y - obj->centre.y ) );
			distance = 1.0 - distance / ( mirror_b * 2.0 );
			assert( distance >= 0 && distance <= 1 );
			match_scores->distance[ c ][ o ] = distance;



			//---- area difference measure
			double min_area = MIN( obj->area, cls->area );
			double max_area = MAX( obj->area, cls->area );
			double area_ratio = min_area / max_area;
			match_scores->area[ c ][ o ] = area_ratio;



			//---- bhattacharyya measure
			double bd = bhattacharyyaCoefficient(	cls->h_bins, obj->h_bins,
													cls->s_bins, obj->s_bins,
													cls->v_bins, obj->v_bins );
			match_scores->colour[ c ][ o ] = bd * bd;



			// now eliminate the 'potential' matches whose similarity 
			// measures are above or below certain limits
			bool allow_match = true;

			double max_obj_distance = ( 1.0 - cfg->getMaxBlobFrameMovement() )
									  - MAX( obj->bounding_box.width / ( mirror_b * 2.0 ), obj->bounding_box.height / ( mirror_b * 2.0 ) );

			if ( match_scores->overlap[ c ][ o ] == 0 && 
				 match_scores->distance[ c ][ o ] < max_obj_distance )
			{
				allow_match = false;
			}


			if ( match_scores->overlap[ c ][ o ] == 0 && 
				 match_scores->area[ c ][ o ] < 0.2 && 
				 abs( obj->area - cls->area ) > 2 * cfg->getMinBlobArea() ) 
			{
				allow_match = false;
			}


			// allowed?
			if ( ! allow_match )
			{
				match_scores->overlap[ c ][ o ] = -1;
				match_scores->distance[ c ][ o ] = -1;
				match_scores->area[ c ][ o ] = -1;
				match_scores->colour[ c ][ o ] = -1;
			}
		}
	}

	match_scores->sumTotalScore( objects );
}




void dumpMatchScoreTable( long fr, MatchScores* match_scores, ObjectList* clusters, ClusterObjectGraph* cluster_objects, ObjectList* objects )
{
	char tmp[100];
	sprintf( tmp, "\\out\\%d_match.txt", fr );
	FILE* f = fopen( tmp, "w+" );
	assert( f );


	fprintf( f, "OVERLAP\n" );
	fprintf( f, "     \to0\to1\to2\to3\to4\to5\to6\to7\to8\to9\to10\to11\to12\to13\t \n" );
	for ( int c = 0; c < 20; ++c )
	{
		fprintf( f, "B%02.02d:\t", c );
		for ( int o = 0; o < 20; ++o )
		{
			double v = match_scores->overlap[ c ][ o ];
			if ( v == 0 || v > 9999 )
				fprintf( f, "-\t" );
			else
				fprintf( f, "%f\t", v );
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n\n" );


	fprintf( f, "DISTANCE\n" );
	fprintf( f, "     \to0\to1\to2\to3\to4\to5\to6\to7\to8\to9\to10\to11\to12\to13\t \n" );
	for ( int c = 0; c < 20; ++c )
	{
		fprintf( f, "B%02.02d:\t", c );
		for ( int o = 0; o < 20; ++o )
		{
			double v = match_scores->distance[ c ][ o ];
			if ( v == 0 || v > 9999 )
				fprintf( f, "-\t" );
			else
				fprintf( f, "%f\t", v );
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n\n" );


	fprintf( f, "COLOUR\n" );
	fprintf( f, "     \to0\to1\to2\to3\to4\to5\to6\to7\to8\to9\to10\to11\to12\to13\t \n" );
	for ( int c = 0; c < 20; ++c )
	{
		fprintf( f, "B%02.02d:\t", c );
		for ( int o = 0; o < 20; ++o )
		{
			double v = match_scores->colour[ c ][ o ];
			if ( v == 0 || v > 9999 )
				fprintf( f, "-\t" );
			else
				fprintf( f, "%f\t", v );
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n\n" );


	fprintf( f, "AREA\n" );
	fprintf( f, "     \to0\to1\to2\to3\to4\to5\to6\to7\to8\to9\to10\to11\to12\to13\t \n" );
	for ( int c = 0; c < 20; ++c )
	{
		fprintf( f, "B%02.02d:\t", c );
		for ( int o = 0; o < 20; ++o )
		{
			double v = match_scores->area[ c ][ o ];
			if ( v == 0 || v > 9999 )
				fprintf( f, "-\t" );
			else
				fprintf( f, "%f\t", v );
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n\n" );


	fprintf( f, "\nTOTAL\n" );
	fprintf( f, "     \to0\to1\to2\to3\to4\to5\to6\to7\to8\to9\to10\to11\to12\to13\t \n" );
	for ( int c = 0; c < 20; ++c )
	{
		fprintf( f, "B%02.02d:\t", c );
		for ( int o = 0; o < 20; ++o )
		{
			double v = match_scores->total[ c ][ o ];
			if ( v == 0 || v > 9999 )
				fprintf( f, "-\t" );
			else
				fprintf( f, "%f\t", v );
		}
		fprintf( f, "\n" );
	}
	fprintf( f, "\n\n" );


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );
		fprintf( f, "o%d:\tmerged=%d\tlost=%d\toccluded=%d\t\t%d\t\t", o, obj->merged, obj->lost, obj->occluded, obj->lives ); 

		for ( int cls = cluster_objects->firstLinkB( o ); cls != -1; cls = cluster_objects->nextLinkB( o, cls ) )
			fprintf( f, "%d", cls );

		fprintf( f, "\n" );
	}

	fclose( f );
}




void convert_base( int v, int base, int digits[], int num_digits )
{
	int d, r;
	int k = 0;

	do
	{
		d = v % base;
		r = v / base;

		assert( k < num_digits );
		digits[ k++ ] = d;
		v = r;
	}
	while( r > 0 );

	while( k < num_digits )
		digits[ k++ ] = 0;
}




void matchClusters2Objects(	 ObjectList* clusters, 
							 ObjectList* objects,
							 ClusterObjectGraph* cluster_objects,
							 MatchScores* match_scores,
							 long frame )
{
	if ( clusters->size() && objects->size() )
	{
		unsigned char* obj_ndx = new unsigned char[ objects->size() ];
		assert( obj_ndx );

		int obj_size = 0;
		for ( int o = objects->first(); o != -1; o = objects->next( o ) )
			obj_ndx[ obj_size++ ] = o;
		assert( obj_size == objects->size() );


		unsigned char* cls_ndx = new unsigned char[ clusters->size() ];
		assert( cls_ndx );
		
		int cls_size = 0;
		for ( int c = clusters->first(); c != -1; c = clusters->next( c ) )
			cls_ndx[ cls_size++ ] = c;
		assert( cls_size == clusters->size() );

		
		int total = (long) pow( (double) cls_size, obj_size );
		const int UNKNOWN = -1;
		double best_match_error = 999999;
		int best_match = UNKNOWN;

		for ( int k = 0; k < total; ++k )
		{
			int* out = new int[ objects->size() ];
			assert( out );

			convert_base( k, cls_size, out, obj_size );

			double match_error = 0;

			bool* clusters_used = new bool[ clusters->size() ];
			assert( clusters_used );

			for ( int i = 0; i < cls_size; ++i )
				clusters_used[ i ] = false;

			for ( int i = 0; i < obj_size; ++i )
			{
				clusters_used[ out[ i ] ] = true;
				match_error += MatchScores::TOTAL_WEIGHT 
							   - match_scores->total[ cls_ndx[ out[ i ] ] ][ obj_ndx[ i ] ];
			}

			// now check if all blob clusters were used
			for ( int i = 0; i < cls_size; ++i )
			{
				if ( ! clusters_used[ i ] )
					match_error += 2.0;
			}

			if ( match_error < best_match_error )
			{
				best_match_error = match_error;
				best_match = k;
			}

			delete[] out;
			delete[] clusters_used;
		}

		if ( best_match != UNKNOWN )
		{
			int* out = new int[ objects->size() ];
			assert( out );

			convert_base( best_match, cls_size, out, obj_size );

			for ( int i = 0; i < obj_size; ++i )
			{
				if ( match_scores->distance[ cls_ndx[ out[ i ] ] ][ obj_ndx[ i ] ] > 0 )
					cluster_objects->link( cls_ndx[ out[ i ] ], obj_ndx[ i ] );
			}

			delete[] out;
		}

		delete[] obj_ndx;
		delete[] cls_ndx;
	}


#ifdef DUMP_MATCH_SCORE_TABLE
	dumpMatchScoreTable( frame, match_scores, clusters, cluster_objects, objects );
#endif
}




void removeGhosts(	BlobList* blobs,
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects, 
					IplImage* lbl_map,
					IplImage* bkg_rgb_mean,
					IplImage* bkg_hsv_mean,
					IplImage* src,
					IplImage* src_hsv,
					long frame, IplImage* out )
{
	assert( lbl_map );
	assert( lbl_map->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( src );
	assert( src->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( src_hsv );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );


	CfgParameters* cfg = CfgParameters::instance();

	
	double mirror_b2 = cfg->getMirrorBoundary() * 2;
	double dist_threshold = mirror_b2 * cfg->getMaxBlobFrameMovement();


	for ( int o1 = objects->first(); o1 != -1; o1 = objects->next( o1 ) )
	{
		Object* obj1 = objects->getAt( o1 );

		if ( ! obj1->merged && ! obj1->lost && ! obj1->occluded )
		{
			double dist_threshold_obj = dist_threshold
									  + MAX( obj1->bounding_box.width, obj1->bounding_box.height );

			for ( int o2 = objects->first(); o2 != -1; o2 = objects->next( o2 ) )
			{
				if ( o2 != o1 )
				{
					Object* obj2 = objects->getAt( o2 );

					if ( ! obj2->merged && ! obj2->lost && ! obj2->occluded 
						 && obj2->lives > 15 )
					{
						CvPoint2D32f start1 = obj1->first_pos;
						CvPoint2D32f start2 = obj2->first_pos;
						
						double dp = sqrt( SQR( start1.x - start2.x ) + SQR( start1.y - start2.y ) );

						if ( dp < dist_threshold_obj )
						{
							CvPoint2D32f now1 = obj1->centre;
							CvPoint2D32f now2 = obj2->centre;

							double dm1 = sqrt( SQR( now1.x - start1.x ) + SQR( now1.y - start1.y ) );
							double dm2 = sqrt( SQR( now2.x - start2.x ) + SQR( now2.y - start2.y ) );
							double dm1b = sqrt( SQR( now1.x - start2.x ) + SQR( now1.y - start2.y ) );
							double dm2b = sqrt( SQR( now2.x - start1.x ) + SQR( now2.y - start1.y ) );


							if ( ( dm1 > 20 && dm2 < 20 ) || ( dm1b > 20 && dm2b < 20 ) ) 
							{
								double h_bins[ Object::H_NUM_BINS ];
								double s_bins[ Object::S_NUM_BINS ];
								double v_bins[ Object::V_NUM_BINS ];

								for ( int k = 0; k < Object::H_NUM_BINS; ++k )
									h_bins[ k ] = 0;
								for ( int k = 0; k < Object::S_NUM_BINS; ++k )
									s_bins[ k ] = 0;
								for ( int k = 0; k < Object::V_NUM_BINS; ++k )
									v_bins[ k ] = 0;


								for ( int c = cluster_objects->firstLinkB( o2 ); c != -1; c = cluster_objects->nextLinkB( o2, c ) )
								{
									Object* cls = clusters->getAt( c );
								
									long area = 0;


									for ( int b = blob_clusters->firstLinkB( c ); b != -1; b = blob_clusters->nextLinkB( c, b ) )
									{
										Blob* blob = blobs->getAt( b );

										
										int xmin = blob->bounding_box.x;
										int xmax = blob->bounding_box.x + blob->bounding_box.width;
										int ymin = blob->bounding_box.y;
										int ymax = blob->bounding_box.y + blob->bounding_box.height;

										for ( int y = ymin; y <= ymax; ++y )
										{
											COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );
											COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y );

											for ( int x = xmin, x3 = xmin*3; x <= xmax; ++x, x3 +=3 )
											{
												if ( p_lbl_map[ x ] == blob->label )
												{
													++area;

													int hi = p_bkg_hsv_mean[ x3    ] / 1000;
													int si = p_bkg_hsv_mean[ x3 +1 ] / 1000;
													int vi = p_bkg_hsv_mean[ x3 +2 ] / 1000;


													// HSV histogram
													hi = hi >= 360 ? hi - 360 : hi;
													int hh = hi / ( 360.0 / Object::H_NUM_BINS );
													int ss = si / ( 256.0 / Object::S_NUM_BINS );
													int vv = vi / ( 256.0 / Object::V_NUM_BINS );

													assert( hh >= 0 && hh < Object::H_NUM_BINS );
													assert( ss >= 0 && ss < Object::S_NUM_BINS );
													assert( vv >= 0 && vv < Object::V_NUM_BINS );

													++h_bins[ hh ];
													++s_bins[ ss ];
													++v_bins[ vv ];
												}
											}
										}
									}

									for ( int k = 0; k < Object::H_NUM_BINS; ++k )
										h_bins[ k ] /= area;
									for ( int k = 0; k < Object::S_NUM_BINS; ++k )
										s_bins[ k ] /= area;
									for ( int k = 0; k < Object::V_NUM_BINS; ++k )
										v_bins[ k ] /= area;
								}


								//---- bhattacharyya measure
								double bd = bhattacharyyaCoefficient(	h_bins, obj2->h_bins,
																		s_bins, obj2->s_bins,
																		v_bins, obj2->v_bins );
								bd = bd * bd;

								if ( bd > 0.5 )
								{
									for ( int c = cluster_objects->firstLinkB( o2 ); c != -1; c = cluster_objects->nextLinkB( o2, c ) )
									{
										Object* cls = clusters->getAt( c );

										for ( int b = blob_clusters->firstLinkB( c ); b != -1; b = blob_clusters->nextLinkB( c, b ) )
										{
											Blob* blob = blobs->getAt( b );

											
											int xmin = blob->bounding_box.x;
											int xmax = blob->bounding_box.x + blob->bounding_box.width;
											int ymin = blob->bounding_box.y;
											int ymax = blob->bounding_box.y + blob->bounding_box.height;

											for ( int y = ymin; y <= ymax; ++y )
											{
												COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );
												COMPUTE_IMAGE_ROW_PTR( int, p_bkg_rgb_mean, bkg_rgb_mean, y );
												COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y );
												COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y );
												COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y );

												for ( int x = xmin, x3 = xmin*3; x <= xmax; ++x, x3 +=3 )
												{
													if ( p_lbl_map[ x ] == blob->label )
													{
														// patch background hole with pixels from current frame
														p_bkg_rgb_mean[ x3 +2 ] = p_src[ x3 +2 ] * 1000;
														p_bkg_rgb_mean[ x3 +1 ] = p_src[ x3 +1 ] * 1000;
														p_bkg_rgb_mean[ x3    ] = p_src[ x3    ] * 1000;

														p_bkg_hsv_mean[ x3    ] = p_src_hsv[ x3    ] * 1000;
														p_bkg_hsv_mean[ x3 +1 ] = p_src_hsv[ x3 +1 ] * 1000;
														p_bkg_hsv_mean[ x3 +2 ] = p_src_hsv[ x3 +2 ] * 1000;
													}
												}
											}
										}
									}

									// finally remove the ghost object
									objects->remove( o2 );
								}

							}
						}
					}
				}
			}
		}
	}
}




void dumpObjects(	BlobList* blobs,
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects, 
					IplImage* lbl_map,
					IplImage* src,
					long frame )
{
	assert( lbl_map );
	assert( lbl_map->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( src );
	assert( src->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );


	char tmp[200];
	IplImage* out = cvCreateImage( cvSize( src->width, src->height ), IPL_DEPTH_8U, 3 );
	assert( out );

	
	for ( int o1 = objects->first(); o1 != -1; o1 = objects->next( o1 ) )
	{
		Object* obj1 = objects->getAt( o1 );

		sprintf( tmp, "\\out\\%04.04d_obj%02.02d.bmp", frame, o1 );
		iplSet( out, 0 );

		for ( int c = cluster_objects->firstLinkB( o1 ); c != -1; c = cluster_objects->nextLinkB( o1, c ) )
		{
			Object* cls = clusters->getAt( c );
								
			for ( int b = blob_clusters->firstLinkB( c ); b != -1; b = blob_clusters->nextLinkB( c, b ) )
			{
				Blob* blob = blobs->getAt( b );
				
				int xmin = blob->bounding_box.x;
				int xmax = blob->bounding_box.x + blob->bounding_box.width;
				int ymin = blob->bounding_box.y;
				int ymax = blob->bounding_box.y + blob->bounding_box.height;

				for ( int y = ymin; y <= ymax; ++y )
				{
					COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y );
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y );
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_out, out, y );

					for ( int x = xmin, x3 = xmin*3; x <= xmax; ++x, x3 +=3 )
					{
						if ( p_lbl_map[ x ] == blob->label )
						{
							p_out[ x3    ] = p_src[ x3    ];
							p_out[ x3 +1 ] = p_src[ x3 +1 ];
							p_out[ x3 +2 ] = p_src[ x3 +2 ];
						}
					}
				}
			}
		}

		cvSaveImage( tmp, out );
	}

	cvReleaseImage( &out );
}




void showConvexHull( IplImage* src, IplImage* out, CvRect roi, CvScalar outline_colour )
{
	assert( src );
	assert( out );
	assert( src->nChannels == 1 );
	assert( src->depth == IPL_DEPTH_32F );
	assert( out->nChannels == 3 );
	assert( out->depth == IPL_DEPTH_8U );

	int max_pts = roi.height * 2;
	assert( max_pts < 2000 );

	int count = -1;
	

	CvPoint* points = new CvPoint[ max_pts ];
	int* hull = new int[ max_pts ];


	for ( int y = roi.y; y < roi.y + roi.height; ++y )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_src, src, y );
		COMPUTE_IMAGE_ROW_PTR( unsigned char, p_out, out, y );

		int min_x, max_x;
 		for ( min_x = roi.x; min_x < ( roi.x + roi.width ) && p_src[ min_x ] == 0.0; ++min_x )
		{}

		for ( max_x = roi.x + roi.width; max_x >= roi.x && p_src[ max_x ] == 0.0; --max_x )
		{}

		
		bool start = false, end = false;

		if ( p_src[ min_x ] > 0.0 )
		{
			++count;
			assert( count <= max_pts );
			
			points[ count ].x = min_x;
			points[ count ].y = y;

			start = true;
		}

		if ( p_src[ max_x ] > 0.0 )
		{
			++count;
			assert( count <= max_pts );

			points[ count ].x = max_x;
			points[ count ].y = y;

			end = true;
		}

#ifdef FILL_CONVEX_HULL
		if ( start && end )
		{
			for ( int x = min_x * 3; x <= max_x * 3; x+=3 )
			{
				p_out[ x ] = 255;
				p_out[ x+1 ] = 255;
				p_out[ x+2 ] = 255;
			}
		}
#endif
	}

	if ( count == -1 )
		return;


	CvMat pointMat = cvMat( 1, count, CV_32SC2, points );
	CvMat hullMat = cvMat( 1, count, CV_32SC1, hull );

	cvConvexHull2( &pointMat, &hullMat, CV_CLOCKWISE, 0 );

	int hullCount = hullMat.cols;

	CvPoint pt0 = points[ hull[ hullCount-1 ] ];
	for ( int i = 0; i < hullCount; ++i )
	{
		CvPoint pt = points[ hull[ i ] ];
		cvLine( out, pt0, pt, outline_colour );
		pt0 = pt;
	}


	delete[] points;
	delete[] hull;
}




void drawOutput( IplImage* out, 
				 BlobList* blobs, 
				 BlobClusterGraph* blob_clusters,
				 ObjectList* clusters,
				 ClusterObjectGraph* cluster_objects,
				 ObjectList* objects,
				 IplImage* lbl_map,
				 IplImage* src,
				 long frame )
{
	assert( out->nChannels == 3 );
	assert( out->depth == IPL_DEPTH_8U );


	// show a faint version source image
	iplMultiplySScale( src, out, 100 );


	// display foreground pixels
	int y = src->height;
	do
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y -1 );
		COMPUTE_IMAGE_ROW_PTR( unsigned char, p_out, out, y -1 );

		int x = src->width;
		do
		{
			if ( p_lbl_map[ x-1 ] != 0 )
			{
				int r = p_lbl_map[ x-1 ] -1;
				if ( ! blobs->isFree( r ) )
				{
					int x3 = (x-1) * 3;

					p_out[ x3++ ] = 255;
					p_out[ x3++ ] = 255;
					p_out[ x3++ ] = 255;
				}
			}
		}
		while( --x );
	}
	while( --y );


#ifdef SHOW_CONVEX_HULL
	{
		for ( int o = objects->first(); o != -1; o = objects->next( o ) )
		{
			Object* obj = objects->getAt( o );

			if ( ! obj->occluded && ! obj->merged && ! obj->lost )
				showConvexHull( lbl_map, out1.img, obj->bounding_box );
		}
	}
#endif


	// show object label IDs
	static CvFont my_font;
	static bool once = true;
	if ( once )
	{
		cvInitFont( &my_font, CV_FONT_VECTOR0, 0.5, 0.5, 0, 2 );
		once = false;
	}


	drawObjectBoundingBox( out, objects, frame );


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		char tmp[ 10 ];
		sprintf( tmp, "%d", obj->label );
		cvPutText( out, tmp, cvPoint( obj->centre.x + obj->label, 
									  obj->centre.y + obj->label ),
				   &my_font, CV_RGB(0,0,255) );
	}


#ifdef SHOW_FRAME_NUMBER
	{
		char tmp[ 15 ];
		sprintf( tmp, "%d", frame );
		cvPutText(	out, tmp, cvPoint( 5, src->height-20 ),
					&my_font, 0xffffff );
	}
#endif
}


