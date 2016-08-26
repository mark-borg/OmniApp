
#include <math.h>
#include "Tracker.h"
#include "CfgParameters.h"
#include "..\tools\trig.h"




void drawObjectBoundingBox( IplImage* out, ObjectList* objects, long frame )
{
	int mirror_cx = CfgParameters::instance()->getMirrorCentreX();
	int mirror_cy = CfgParameters::instance()->getMirrorCentreY();

	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		// determine colour from the object's state
		int gry = obj->lost ? 80 : 255;
		CvScalar col = CV_RGB( gry, gry, gry );

#ifdef DRAW_SIMPLE_BOX
		cvRectangle( out, cvPoint( obj->bounding_box.x,obj->bounding_box.y ), 
						  cvPoint( obj->bounding_box.x + obj->bounding_box.width, 
								   obj->bounding_box.y + obj->bounding_box.height ), 
								   col, 1 );
#else
		int c0 = obj->min_radius;
		int c1 = obj->max_radius;

		// in case we have an object right in the middle of the picture - usually
		// happens only if there is motion that causes the edges of the central
		// obstruction to move. in such cases, don't display bounding box.
		if ( c0 < 0 )
			continue;
		
		float z[2], sinz[2], cosz[2];
		z[0] = obj->min_azimuth;
		z[1] = obj->max_azimuth;


		// there's a bug somewhere in this routine that is causing a 360-degree
		// bounding box to be drawn for objects that cross the 0-360 degree
		// boundary. for now, just disable drawing of the box, until bug is
		// found and killed.
		if ( fabs( z[0] - z[1] ) > 360 )
			continue;


		cvbSinCos_32f( z, sinz, cosz, 2, true );

		int x1 = c0 * cosz[ 0 ] + mirror_cx;
		int y1 = c0 * sinz[ 0 ] + mirror_cy;

		int x2 = c1 * cosz[ 0 ] + mirror_cx;
		int y2 = c1 * sinz[ 0 ] + mirror_cy;

		int x3 = c1 * cosz[ 1 ] + mirror_cx;
		int y3 = c1 * sinz[ 1 ] + mirror_cy;

		int x4 = c0 * cosz[ 1 ] + mirror_cx;
		int y4 = c0 * sinz[ 1 ] + mirror_cy;

		cvLine( out, cvPoint( x1, y1 ), cvPoint( x2, y2 ), col, 1 );
		cvLine( out, cvPoint( x3, y3 ), cvPoint( x4, y4 ), col, 1 );
		cvEllipse( out, cvPoint( mirror_cx, mirror_cy ),
				   cvSize( c0, c0 ), 0, - obj->min_azimuth, - obj->max_azimuth, col );
		cvEllipse( out, cvPoint( mirror_cx, mirror_cy ),
				   cvSize( c1, c1 ), 0, - obj->min_azimuth, - obj->max_azimuth, col );		
#endif
	}
}




void nearestDistance( IplImage* blob_map, 
					  int label1, int label2,
					  CvRect rect1, CvRect rect2,
  					  int max_distance,
					  double& nearest_distance,
					  CvPoint& nearest_point )
{
	assert( blob_map->depth == IPL_DEPTH_32F );
	assert( label1 > 0 );
	assert( label2 > 0 );


	// are bounding boxes near each other?
	int xmin = MAX( rect1.x, rect2.x );
	int xmax = MIN( rect1.x + rect1.width, rect2.x + rect2.width );
	int ymin = MAX( rect1.y, rect2.y );
	int ymax = MIN( rect1.y + rect1.height, rect2.y + rect2.height );

	if ( xmin > ( xmax + max_distance ) )
	{
		// don't waste time doing distance transform, but return immediately
		nearest_distance = max_distance;
		nearest_point = cvPoint( -1, -1 );
		return;
	}
	if ( ymin > ( ymax + max_distance ) )
	{
		// don't waste time doing distance transform, but return immediately
		nearest_distance = max_distance;
		nearest_point = cvPoint( -1, -1 );
		return;
	}


	// find union of bounding boxes of the two blobs
	xmin = MIN( rect1.x, rect2.x );
	xmax = MAX( rect1.x + rect1.width, rect2.x + rect2.width );
	ymin = MIN( rect1.y, rect2.y );
	ymax = MAX( rect1.y + rect1.height, rect2.y + rect2.height );

	CvSize size = cvSize( xmax - xmin +1, ymax - ymin +1 );


	IplImage* R1 = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	iplSet( R1, 255 );

	IplImage* R2 = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	iplSet( R2, 0 );


	// create a mask consisting of "NOT R1"
	for ( int y = rect1.y; y <= ( rect1.y + rect1.height ); ++y )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_blob_map, blob_map, y );
		COMPUTE_IMAGE_ROW_PTR( unsigned char, p_R1, R1, y - ymin );

		for ( int x = rect1.x; x <= ( rect1.x + rect1.width ); ++x )
		{
			unsigned char BLACK = 0;

			if ( p_blob_map[ x ] == label1 )
				p_R1[ x - xmin ] = BLACK;
		}
	}


	// create a mask consisting of "R2"
	for ( int y = rect2.y; y <= ( rect2.y + rect2.height ); ++y )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_blob_map, blob_map, y );
		COMPUTE_IMAGE_ROW_PTR( unsigned char, p_R2, R2, y - ymin );

		for ( int x = rect2.x; x <= ( rect2.x + rect2.width ); ++x )
		{
			unsigned char WHITE = 255;

			if ( p_blob_map[ x ] == label2 )
				p_R2[ x - xmin ] = WHITE;
		}
	}


	// calculate distance transform of "NOT R1" from "R1"
	IplImage* dst = cvCreateImage( size, IPL_DEPTH_32F, 1 );
	cvDistTransform( R1, dst );


	// find nearest distance from "NOT R1" by applying "R2" image as mask
	double minv, maxv;
	CvPoint minloc, maxloc;
 	cvMinMaxLoc( dst, &minv, &maxv, &minloc, &maxloc, R2 );

	minloc.x += xmin;
	minloc.y += ymin;

	cvReleaseImage( &dst );
	cvReleaseImage( &R1 );
	cvReleaseImage( &R2 );

	nearest_distance = minv;
	nearest_point = minloc;
}




void labelBlobs( IplImage* lbl_map, 
			     int unlabeled_value,
			     BlobList* blobs,
			     IplImage* mask,
				 int minimum_area )
{
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( lbl_map->nChannels == 1 );
	assert( blobs );

	bool using_mask = ( mask != 0 );
	bool using_area = ( minimum_area != 0 );


	int y = lbl_map->height;
	do
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y-1 );

		unsigned char* p_mask = 0;
		if ( using_mask )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );
			p_mask = p_msk;
		}

		int x = lbl_map->width;
		do
		{
			if ( using_mask && p_mask[ x-1 ] || ! using_mask )
			{
				if ( p_lbl_map[ x-1 ] == unlabeled_value )							
				{
					int blob_ndx = blobs->getNextFreeIndex();
					assert( blob_ndx != -1 );

					int blob_label = blob_ndx + 1;

					CvConnectedComp component;
					CvPoint seed = cvPoint( x-1, y-1 );
					cvFloodFill( lbl_map, seed, cvScalar(blob_label), cvScalarAll(0), cvScalarAll(0), &component, 8 );

					if ( using_area && component.area < minimum_area )
					{
						// restore back the region
						cvFloodFill( lbl_map, seed, cvScalar(unlabeled_value), cvScalarAll(0), cvScalarAll(0), &component, 8 );
					}
					else
					{
						Blob b;
						b.bounding_box  = component.rect;
						b.label			= blob_label;
						b.seed			= seed;
						b.area			= component.area;

						int i = blobs->add( b );
						assert( i == blob_ndx );
					}
				}
			}
		}
		while( --x );
	}
	while( --y );
}



