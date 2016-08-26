
#include <malloc.h>
#include <math.h>
#include "ColourTracker.h"
#include "CfgParameters.h"
#include "Log.h"
#include "..\tools\trig.h"
#include "..\tools\float_cast.h"
#include "..\tools\timer.h"
#include <highgui.h>
 




//#define DRAW_SIMPLE_BOX
#define SHOW_FRAME_NUMBER
//#define SHOW_OBJECT_PATH



// these are only used for debug purposes...
//#define DUMP_EM_RESULTS
//#define DUMP_OBJ_PROBABILITY_RESULT
//#define DUMP_LABELLING_MAP




// some constants that never change
#define HALF_PROB	0.5
#define NUM_SIGMA	2.5
#define COMPONENT_PROB_THRESHOLD	1e-4




void drawOutput2( IplImage* out, 
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


	const int colours[][3] = {	{ 0, 0, 255 }, { 0, 255, 0 }, { 255, 0, 0 },
								{ 255, 255, 0 }, { 255, 0, 255 }, { 0, 255, 255 },
								{ 0, 0, 200 }, { 0, 200, 0 }, { 200, 0, 0 },
								{ 200, 200, 0 }, { 200, 0, 200 }, { 0, 200, 200 }	};

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
			int l = p_lbl_map[ x-1 ];
			if ( l )
			{
				int x3 = (x-1) * 3;

				if ( l == 99999 || l < 2000 )
				{
					p_out[ x3    ] = 100;
					p_out[ x3 +1 ] = 100;
					p_out[ x3 +2 ] = 100;
				}
				else if ( l >= 4000 )
				{
					p_out[ x3    ] = 255;
					p_out[ x3 +1 ] = 255;
					p_out[ x3 +2 ] = 255;
				}
				else //if ( l >= 2000 )
				{
					l = l - 2000;
					p_out[ x3    ] = colours[ l % 12 ][0];
					p_out[ x3 +1 ] = colours[ l % 12 ][1];
					p_out[ x3 +2 ] = colours[ l % 12 ][2];
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
		cvInitFont( &my_font, CV_FONT_VECTOR0, 1, 1, 0, 2 );
		once = false;
	}


	drawObjectBoundingBox( out, objects, frame );


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		char tmp[ 10 ];
		sprintf( tmp, "%d", obj->label );
		cvPutText( out, tmp, cvPoint( obj->centre.x, 
									  obj->centre.y ),
				   &my_font, CV_RGB(0,0,240) );
	}



#ifdef SHOW_OBJECT_PATH
	// display path of objects on the omnidirectional image
	{
		for ( int o = objects->first(); o != -1; o = objects->next( o ) )
		{
			Object* obj = objects->getAt( o );
			
			if ( obj->path && obj->path->size() > 1 )
			{
				int h = obj->path->first();
				if ( h < obj->path->getNextFreeIndex() )
				{
					CvPoint2D32f *pos2, *pos1 = obj->path->getAt( h );

					for ( ; h < obj->path->getNextFreeIndex(); ++h )
					{
						pos2 = obj->path->getAt( h );
						
						cvLine( out, cvPoint( pos1->x, pos1->y ), 
									 cvPoint( pos2->x, pos2->y ),
									 CV_RGB(200,200,200) );
						pos1 = pos2;
					}
				}
			}
		}
	}
#endif


#ifdef SHOW_FRAME_NUMBER
	{
		char tmp[ 15 ];
		sprintf( tmp, "%d", frame );
		cvPutText(	out, tmp, cvPoint( 5, src->height-20 ),
					&my_font, CV_RGB(255,255,255) );		//MM! 0xffffff );
	}
#endif

}




#define FLIP_FLOP_RATE	1



void fitColourGMM( 	ObjectList* objects,
					int obj_id,
					IplImage* lbl_map,
					IplImage* src,
					IplImage* src_hsv,
					long frame )
{
	assert( objects );
	assert( lbl_map );
	assert( lbl_map->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( src );
	assert( src->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( src_hsv );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );


	Object* obj = objects->getAt( obj_id );
	assert( obj );


	const int VECTOR_GROWTH = 1000;
	double* points_x = (double*) malloc( VECTOR_GROWTH * sizeof( double ) );
	double* points_y = (double*) malloc( VECTOR_GROWTH * sizeof( double ) );


	long count = 0;
	int flipflop = 0;


#ifdef DUMP_EM_RESULTS
	IplImage* out1 = iplCloneImage( src );
	iplSet( out1, 0 );
#endif



	CvRect bbox = obj->bounding_box;

	if ( bbox.width == 0 || bbox.height == 0 )
		return;
	
	int grow_box = CfgParameters::instance()->getNearbyColourThreshold();

	bbox.x -= grow_box;
	bbox.y -= grow_box;
	bbox.width += grow_box * 2;
	bbox.height += grow_box * 2;
	if ( bbox.x < 0 )	bbox.x = 0;
	if ( bbox.y < 0 )	bbox.y = 0;
	if ( bbox.x + bbox.width >= src_hsv->width )
		bbox.width = src_hsv->width - bbox.x -1;
	if ( bbox.y + bbox.height >= src_hsv->height )
		bbox.height = src_hsv->height - bbox.y -1;

	assert( bbox.x >= 0 );
	assert( bbox.y >= 0 );
	assert( bbox.x + bbox.width < lbl_map->width );
	assert( bbox.y + bbox.height < lbl_map->height );


			
			// for all pixels of this object
			int x0 = bbox.x;
			int y0 = bbox.y;

			int y = bbox.height;
			do
			{
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y + y0 -1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y + y0 -1 );
				COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y + y0 -1 );
#ifdef DUMP_EM_RESULTS
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_out1, out1, y + y0 -1 );
#endif

				int x = bbox.width;
				do
				{
					register int xi = x + x0 -1; 

					int l = p_lbl_map[ xi ];
					if ( l == 2000 + obj->label || l == 4000 + obj->label )
					{
						if ( ! flipflop )
						{
							register int xi3 = xi * 3;

							int h = p_src_hsv[ xi3    ];
							int s = p_src_hsv[ xi3 +1 ];
							int v = p_src_hsv[ xi3 +2 ];

							if ( v > LOW_HSV_THRESHOLD_V && s < HIGH_HSV_THRESHOLD_S )
							{
								points_x[ count ] = h;
								points_y[ count++ ] = s;

								if ( count % VECTOR_GROWTH == 0 )
								{
									points_x = (double*) realloc( points_x, _msize( points_x ) + VECTOR_GROWTH * sizeof( double ) );
									points_y = (double*) realloc( points_y, _msize( points_y ) + VECTOR_GROWTH * sizeof( double ) );
								}

#ifdef DUMP_EM_RESULTS
								p_out1[ xi3 +2 ] = p_src[ xi3 +2 ];
								p_out1[ xi3 +1 ] = p_src[ xi3 +1 ];
								p_out1[ xi3    ] = p_src[ xi3    ];
#endif
							}
						}

						flipflop = ++flipflop % FLIP_FLOP_RATE;
					}
				}
				while( --x );
			}
			while( --y );




#ifdef DUMP_EM_RESULTS
	{
		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_pxls.bmp", frame, obj_id );
		cvSaveImage( tmp, out1 );
		iplDeallocate( out1, IPL_IMAGE_ALL );
	}
#endif


	int num_pts = count;
	const float SPACE_SCALE = 1;
	int HS_SPACE_RADIUS = 256 / SPACE_SCALE;



#ifdef DUMP_EM_RESULTS
	{
		IplImage* out2 = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_8U, 3 );
		iplSet( out2, 0 );

		// display object's pixels in HS space
		for ( int p =0; p < count; ++p )
		{
			int h = points_x[ p ];
			int s = points_y[ p ];
			const int v = 200;

			int x = HS_SPACE_RADIUS + s * cos( h * DEG_2_RAD ) / SPACE_SCALE;
			int y = HS_SPACE_RADIUS + s * sin( h * DEG_2_RAD ) / SPACE_SCALE;

			unsigned char r, g, b;
			hsv2rgb( h, s, v, r, g, b );

			unsigned char pxl[] = { b, g, r };
			cvCircle( out2, cvPoint( x, y ), 2, RGB( b, g, r ), -1 );
		}

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_hs_space.bmp", frame, obj_id );
		cvSaveImage( tmp, out2 );
		cvReleaseImage( &out2 );

		
		// for reference, generate the entire HS space slice
		static first_time = true;

		if ( first_time )
		{
			IplImage* out2 = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_8U, 3 );
			iplSet( out2, 0 );

			// display HS space
			const int v = 200;

			for ( int h = 0; h < 360; ++h )
			{
				for ( int s = 0; s < 256; ++s )
				{
					int x = HS_SPACE_RADIUS + s * cos( h * DEG_2_RAD ) / SPACE_SCALE;
					int y = HS_SPACE_RADIUS + s * sin( h * DEG_2_RAD ) / SPACE_SCALE;

					unsigned char r, g, b;
					hsv2rgb( h, s, v, r, g, b );

					unsigned char pxl[] = { b, g, r };
					cvCircle( out2, cvPoint( x, y ), 2, RGB( b, g, r ), -1 );
				}
			}

			char tmp[200];
			sprintf( tmp, "\\out\\hs_space.bmp" );
			cvSaveImage( tmp, out2 );
			cvReleaseImage( &out2 );

			first_time = false;
		}
	}
#endif



	// converting HS colours to circular representation in Cartesian space
	{
		double* X = new double[ num_pts ];
		double* Y = new double[ num_pts ];

		CvMat HM, SM, XM, YM;
		cvInitMatHeader( &HM, 1, num_pts, CV_64F, points_x );
		cvInitMatHeader( &SM, 1, num_pts, CV_64F, points_y );
		cvInitMatHeader( &XM, 1, num_pts, CV_64F, X );
		cvInitMatHeader( &YM, 1, num_pts, CV_64F, Y );

		cvPolarToCart( &SM, &HM, &XM, &YM, 1 );
		
		// scale space & offset centre
		for ( int p = 0; p < count; ++p )
		{
			points_x[ p ] = HS_SPACE_RADIUS + X[ p ] / SPACE_SCALE;
			points_y[ p ] = HS_SPACE_RADIUS + Y[ p ] / SPACE_SCALE;
		}

		delete[] X;
		delete[] Y;
	}



	// initialise GMM
	obj->gmm.reset();
	const int total_components = Object::GMM_COMPONENTS;

	for ( int g = 0; g < total_components; ++g )
	{
		Object::BivariateGaussian bg;
		bg.set( cvPoint2D32f( points_x[ ( num_pts -1 ) / ( total_components -1 ) *g ],
							  points_y[ ( num_pts -1 ) / ( total_components -1 ) *g ] ),
				cvPoint2D32f( 100*100, 100*100 ), 
				cvPoint2D32f( 0, 0 ) );
		obj->gmm.addComponent( bg, 1.0 / total_components );
	}



	// run the EM algorithm
	EM< CvPoint2D32f, Object::BivariateGaussian, Object::GaussianMixtureModel > 
				em( total_components, count );

	bool converged = false;
	long steps = 0;
	long bailout = 50;

	do
	{
		em.e_step( &obj->gmm, points_x, points_y );
		em.m_step( &obj->gmm, points_x, points_y );
	
		converged = em.hasConverged( &obj->gmm );

		if ( ! converged )
			em.updateParams( &obj->gmm );

	}
	while( ! converged && ++steps < bailout );



#ifdef DUMP_EM_RESULTS
	{	
		// dump GMM parameters to the console
		LOG( "GMM for Object " << obj_id );
		obj->gmm.dump( LogOne() );
		LogOne.dump();

				
		IplImage* out = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_8U, 3 );
		iplSet( out, 0 );
	
		const int colors[] = {	RGB( 0, 0, 255 ), RGB( 0, 255, 0 ), RGB( 255, 0, 0 ),
								RGB( 255, 255, 0 ), RGB( 255, 0, 255 ), RGB( 0, 255, 255 ) };

		// show data points
		for ( int n = 0; n < count; ++n )
		{
			int m = em.whichComponent( n );
			cvCircle( out, cvPoint( points_x[ n ], points_y[ n ] ), 2, 
					  m == -1 ? RGB( 155, 155, 155 ) : colors[ m ], -1 );
		}


		// show gaussian component densities
		static CvFont my_font;
		static bool once = true;
		if ( once )
		{
			cvInitFont( &my_font, CV_FONT_VECTOR0, 0.5, 0.5, 0, 1 );
			once = false;
		}
		
		char tmp[200];


		for ( int m = 0; m < total_components; ++m )
		{
			Object::BivariateGaussian* gc = obj->gmm.getComponent( m );

			if ( obj->gmm.getWeight( m ) > 0.099 )
			{
				// find principal axes of component
				CvPoint2D32f eigenvalues = gc->eigenvalues();
				CvPoint2D32f eigenvector1 = gc->eigenvector( eigenvalues.x );
				CvPoint2D32f eigenvector2 = gc->eigenvector( eigenvalues.y );

				double mx = gc->getMean().x;
				double my = gc->getMean().y;
				cvLine( out, 
						cvPoint( mx, my ),
						cvPoint( mx + 3 * eigenvalues.x * eigenvector1.x,
								 my + 3 * eigenvalues.x * eigenvector1.y ),
						colors[ m ] );

				cvLine( out, 
						cvPoint( mx, my ),
						cvPoint( mx + 3 * eigenvalues.y * eigenvector2.x,
								 my + 3 * eigenvalues.y * eigenvector2.y ),
						colors[ m] );

				double angle = atan2( - eigenvector1.y, eigenvector1.x ) * RAD_2_DEG;

				for ( int s = 1; s < 4; ++s )
					cvEllipse(	out, 
								cvPoint( mx, my ), 
								cvSize( s * eigenvalues.x, s * eigenvalues.y ), 
								angle, 0, 360, colors[ m ] );

				
				// display weights of components
				sprintf( tmp, "%02.02f", obj->gmm.getWeight( m ) );
				cvPutText( out, tmp, cvPoint( 10, 20 + 18 * m ),
						   &my_font, colors[ m ] );
			}
		}


		// display the combined 3-sigma bounding box for all components
		CvRect roi = obj->gmm.getSigmaROI();
		cvRectangle( out, cvPoint( roi.x -1, roi.y - 1 ), 
						  cvPoint( roi.x + roi.width +1, roi.y + roi.height +1), 
						  RGB( 100, 100, 100 ) );
		

		sprintf( tmp, "\\out\\%04.04d_object%02.02d_gmm.bmp", frame, obj_id );
		cvSaveImage( tmp, out );


		// show probability in HS-space
		iplSet( out, 0 );
		IplImage* out2 = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_32F, 3 );

		for ( int y = 0; y < 2*HS_SPACE_RADIUS; ++y )
		{
			COMPUTE_IMAGE_ROW_PTR( float, p_out2, out2, y );

			for ( int x = 0; x < 2*HS_SPACE_RADIUS; ++x )
			{
				double p = obj->gmm.probThr( x, y );				

				p_out2[ x*3    ] = p;
				p_out2[ x*3 +1 ] = p;
				p_out2[ x*3 +2 ] = p;
			}
		}

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvSetImageCOI( out2, 1 );
		cvMinMaxLoc( out2, &minv, &maxv, &minpt, &maxpt );
		cvSetImageCOI( out2, 0 );
		cvConvertScale( out2, out, 255.0 / maxv, 0 );


		sprintf( tmp, "\\out\\%04.04d_object%02.02d_gmm2.bmp", frame, obj_id );
		cvSaveImage( tmp, out );


		cvReleaseImage( &out );
		cvReleaseImage( &out2 );
	}
#endif


	free( points_x );
	free( points_y );
	points_x = 0;
	points_y = 0;


	// calculate thresholds for each component
	double thr[ Object::GMM_COMPONENTS ];

	for ( int m = 0; m < total_components; ++m )
	{
		Object::BivariateGaussian* gc = obj->gmm.getComponent( m );
		double w = obj->gmm.getWeight( m );

		// find principal axes of component
		CvPoint2D32f eigenvalues = gc->eigenvalues();
		CvPoint2D32f eigenvector1 = gc->eigenvector( eigenvalues.x );

		double mx = gc->getMean().x;
		double my = gc->getMean().y;

		CvPoint2D32f fv1 = cvPoint2D32f( mx + NUM_SIGMA * eigenvalues.x * eigenvector1.x,
										 my + NUM_SIGMA * eigenvalues.x * eigenvector1.y );

		thr[ m ] = w * gc->prob( fv1.x, fv1.y );
		if ( thr[ m ] < COMPONENT_PROB_THRESHOLD )
			thr[ m ]  = COMPONENT_PROB_THRESHOLD;
	}

	obj->gmm.setThresholds( thr );



	// create probability LUT map for this GMM
	obj->gmm.initMap( 360, 256 );
	CvRect gmm_roi = obj->gmm.getSigmaROI( 3.0 );

	for ( int s = 0; s < 256; ++s )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_map, obj->gmm.getMap(), s );

		for ( int h = 0; h < 360; ++h )
		{
			int x = HS_SPACE_RADIUS + s * cos( h * DEG_2_RAD ) / SPACE_SCALE;
			int y = HS_SPACE_RADIUS + s * sin( h * DEG_2_RAD ) / SPACE_SCALE;

			if ( x >= gmm_roi.x && x <= gmm_roi.x + gmm_roi.width &&
				 y >= gmm_roi.y && y <= gmm_roi.y + gmm_roi.height )
			{
				p_prob_map[ h ] = obj->gmm.probThr( x, y );
			}
			else
			{
				p_prob_map[ h ] = 0.0;
			}
		}
	}


	obj->gmm_updated = frame;



#ifdef DUMP_EM_RESULTS
	{	
		IplImage* out = cvCreateImage( cvSize( 360, 256 ), IPL_DEPTH_8U, 1 );
		iplSet( out, 0 );

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvSetImageCOI( obj->gmm.getMap(), 1 );
		cvMinMaxLoc( obj->gmm.getMap(), &minv, &maxv, &minpt, &maxpt );
		cvSetImageCOI( obj->gmm.getMap(), 0 );
		cvConvertScale( obj->gmm.getMap(), out, 255.0 / maxv, 0 );

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_prob.bmp", frame, obj_id );
		cvSaveImage( tmp, out );

		cvReleaseImage( &out );
	}
#endif
}




void fitColourGMM( 	BlobList* blobs, 
					BlobClusterGraph* blob_clusters,
					ObjectList* clusters,
					ClusterObjectGraph* cluster_objects,
					ObjectList* objects,
					int obj_id,
					IplImage* lbl_map,
					IplImage* src,
					IplImage* src_hsv,
					long frame )
{
	assert( blobs );
	assert( blob_clusters );
	assert( clusters );
	assert( cluster_objects );
	assert( objects );
	assert( lbl_map );
	assert( lbl_map->nChannels == 1 );
	assert( lbl_map->depth == IPL_DEPTH_32F );
	assert( src );
	assert( src->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( src_hsv );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );


	Object* obj = objects->getAt( obj_id );
	assert( obj );


	const int VECTOR_GROWTH = 1000;
	double* points_x = (double*) malloc( VECTOR_GROWTH * sizeof( double ) );
	double* points_y = (double*) malloc( VECTOR_GROWTH * sizeof( double ) );


	long count = 0;
	int flipflop = 0;


#ifdef DUMP_EM_RESULTS
	IplImage* out1 = iplCloneImage( src );
	iplSet( out1, 0 );
#endif

	
	// for all clusters of this object
	for ( int c = cluster_objects->firstLinkB( obj_id ); c != -1; c = cluster_objects->nextLinkB( obj_id, c ) )
	{
		Object* cls = clusters->getAt( c );

		// for all the blobs of this blob cluster
		for ( int b = blob_clusters->firstLinkB( c ); b != -1; b = blob_clusters->nextLinkB( c, b ) )
		{
			Blob* blob = blobs->getAt( b );

			int x0 = blob->bounding_box.x;
			int y0 = blob->bounding_box.y;

			int y = blob->bounding_box.height;
			do
			{
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y + y0 -1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y + y0 -1 );
				COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, y + y0 -1 );
#ifdef DUMP_EM_RESULTS
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_out1, out1, y + y0 -1 );
#endif

				int x = blob->bounding_box.width;
				do
				{
					register int xi = x + x0 -1; 

					if ( p_lbl_map[ xi ] == blob->label )
					{
						if ( ! flipflop )
						{
							register int xi3 = xi * 3;

							int h = p_src_hsv[ xi3    ];
							int s = p_src_hsv[ xi3 +1 ];
							int v = p_src_hsv[ xi3 +2 ];

							if ( v > LOW_HSV_THRESHOLD_V && s < HIGH_HSV_THRESHOLD_S )
							{
								points_x[ count ] = h;
								points_y[ count++ ] = s;

								if ( count % VECTOR_GROWTH == 0 )
								{
									points_x = (double*) realloc( points_x, _msize( points_x ) + VECTOR_GROWTH * sizeof( double ) );
									points_y = (double*) realloc( points_y, _msize( points_y ) + VECTOR_GROWTH * sizeof( double ) );
								}

#ifdef DUMP_EM_RESULTS
								p_out1[ xi3 +2 ] = p_src[ xi3 +2 ];
								p_out1[ xi3 +1 ] = p_src[ xi3 +1 ];
								p_out1[ xi3    ] = p_src[ xi3    ];
#endif
							}
						}

						flipflop = ++flipflop % FLIP_FLOP_RATE;
					}
				}
				while( --x );
			}
			while( --y );

		}
	}



#ifdef DUMP_EM_RESULTS
	{
		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_pxls.bmp", frame, obj_id );
		cvSaveImage( tmp, out1 );
		iplDeallocate( out1, IPL_IMAGE_ALL );
	}
#endif


	int num_pts = count;
	const float SPACE_SCALE = 1;
	int HS_SPACE_RADIUS = 256 / SPACE_SCALE;



#ifdef DUMP_EM_RESULTS
	{
		IplImage* out2 = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_8U, 3 );
		iplSet( out2, 0 );

		// display HS space
		for ( int p =0; p < count; ++p )
		{
			int h = points_x[ p ];
			int s = points_y[ p ];
			const int v = 200;

			int x = HS_SPACE_RADIUS + s * cos( h * DEG_2_RAD ) / SPACE_SCALE;
			int y = HS_SPACE_RADIUS + s * sin( h * DEG_2_RAD ) / SPACE_SCALE;

			unsigned char r, g, b;
			hsv2rgb( h, s, v, r, g, b );

			unsigned char pxl[] = { b, g, r };
			cvCircle( out2, cvPoint( x, y ), 2, RGB( b, g, r ), -1 );
		}

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_hs_space.bmp", frame, obj_id );
		cvSaveImage( tmp, out2 );
		cvReleaseImage( &out2 );
	}
#endif



	// converting HS colours to circular representation in Cartesian space
	{
		double* X = new double[ num_pts ];
		double* Y = new double[ num_pts ];

		CvMat HM, SM, XM, YM;
		cvInitMatHeader( &HM, 1, num_pts, CV_64F, points_x );
		cvInitMatHeader( &SM, 1, num_pts, CV_64F, points_y );
		cvInitMatHeader( &XM, 1, num_pts, CV_64F, X );
		cvInitMatHeader( &YM, 1, num_pts, CV_64F, Y );

		cvPolarToCart( &SM, &HM, &XM, &YM, 1 );
		
		// scale space & offset centre
		for ( int p = 0; p < count; ++p )
		{
			points_x[ p ] = HS_SPACE_RADIUS + X[ p ] / SPACE_SCALE;
			points_y[ p ] = HS_SPACE_RADIUS + Y[ p ] / SPACE_SCALE;
		}

		delete[] X;
		delete[] Y;
	}



	// initialise GMM
	obj->gmm.reset();
	const int total_components = Object::GMM_COMPONENTS;

	for ( int g = 0; g < total_components; ++g )
	{
		Object::BivariateGaussian bg;
		bg.set( cvPoint2D32f( points_x[ ( num_pts -1 ) / ( total_components -1 ) *g ],
							  points_y[ ( num_pts -1 ) / ( total_components -1 ) *g ] ),
				cvPoint2D32f( 100*100, 100*100 ), 
				cvPoint2D32f( 0, 0 ) );
		obj->gmm.addComponent( bg, 1.0 / total_components );
	}



	// run the EM algorithm
	EM< CvPoint2D32f, Object::BivariateGaussian, Object::GaussianMixtureModel > 
				em( total_components, count );

	bool converged = false;
	long steps = 0;
	long bailout = 50;

	do
	{
		em.e_step( &obj->gmm, points_x, points_y );
		em.m_step( &obj->gmm, points_x, points_y );
	
		converged = em.hasConverged( &obj->gmm );

		if ( ! converged )
			em.updateParams( &obj->gmm );

	}
	while( ! converged && ++steps < bailout );



#ifdef DUMP_EM_RESULTS
	{	
		// dump GMM parameters to the console
		LOG( "GMM for Object " << obj_id );
		obj->gmm.dump( LogOne() );
		LogOne.dump();

				
		IplImage* out = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_8U, 3 );
		iplSet( out, 0 );
	
		const int colors[] = {	RGB( 0, 0, 255 ), RGB( 0, 255, 0 ), RGB( 255, 0, 0 ),
								RGB( 255, 255, 0 ), RGB( 255, 0, 255 ), RGB( 0, 255, 255 ) };

		// show data points
		for ( int n = 0; n < count; ++n )
		{
			int m = em.whichComponent( n );
			cvCircle( out, cvPoint( points_x[ n ], points_y[ n ] ), 2, 
					  m == -1 ? RGB( 155, 155, 155 ) : colors[ m ], -1 );
		}


		// show gaussian component densities
		static CvFont my_font;
		static bool once = true;
		if ( once )
		{
			cvInitFont( &my_font, CV_FONT_VECTOR0, 0.5, 0.5, 0, 1 );
			once = false;
		}
		
		char tmp[200];


		for ( int m = 0; m < total_components; ++m )
		{
			Object::BivariateGaussian* gc = obj->gmm.getComponent( m );

			if ( obj->gmm.getWeight( m ) > 0.099 )
			{
				// find principal axes of component
				CvPoint2D32f eigenvalues = gc->eigenvalues();
				CvPoint2D32f eigenvector1 = gc->eigenvector( eigenvalues.x );
				CvPoint2D32f eigenvector2 = gc->eigenvector( eigenvalues.y );

				double mx = gc->getMean().x;
				double my = gc->getMean().y;
				cvLine( out, 
						cvPoint( mx, my ),
						cvPoint( mx + 3 * eigenvalues.x * eigenvector1.x,
								 my + 3 * eigenvalues.x * eigenvector1.y ),
						colors[ m ] );

				cvLine( out, 
						cvPoint( mx, my ),
						cvPoint( mx + 3 * eigenvalues.y * eigenvector2.x,
								 my + 3 * eigenvalues.y * eigenvector2.y ),
						colors[ m] );

				double angle = atan2( - eigenvector1.y, eigenvector1.x ) * RAD_2_DEG;

				for ( int s = 1; s < 4; ++s )
					cvEllipse(	out, 
								cvPoint( mx, my ), 
								cvSize( s * eigenvalues.x, s * eigenvalues.y ), 
								angle, 0, 360, colors[ m ] );

				
				// display weights of components
				sprintf( tmp, "%02.02f", obj->gmm.getWeight( m ) );
				cvPutText( out, tmp, cvPoint( 10, 20 + 18 * m ),
						   &my_font, colors[ m ] );
			}
		}


		// display the combined 3-sigma bounding box for all components
		CvRect roi = obj->gmm.getSigmaROI();
		cvRectangle( out, cvPoint( roi.x -1, roi.y - 1 ), 
						  cvPoint( roi.x + roi.width +1, roi.y + roi.height +1), 
						  RGB( 100, 100, 100 ) );
		

		sprintf( tmp, "\\out\\%04.04d_object%02.02d_gmm.bmp", frame, obj_id );
		cvSaveImage( tmp, out );


		// show probability in HS-space
		iplSet( out, 0 );
		IplImage* out2 = cvCreateImage( cvSize( 2*HS_SPACE_RADIUS, 2*HS_SPACE_RADIUS ), IPL_DEPTH_32F, 3 );

		for ( int y = 0; y < 2*HS_SPACE_RADIUS; ++y )
		{
			COMPUTE_IMAGE_ROW_PTR( float, p_out2, out2, y );

			for ( int x = 0; x < 2*HS_SPACE_RADIUS; ++x )
			{
				double p = obj->gmm.probThr( x, y );

				p_out2[ x*3    ] = p;
				p_out2[ x*3 +1 ] = p;
				p_out2[ x*3 +2 ] = p;
			}
		}

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvSetImageCOI( out2, 1 );
		cvMinMaxLoc( out2, &minv, &maxv, &minpt, &maxpt );
		cvSetImageCOI( out2, 0 );
		cvConvertScale( out2, out, 255.0 / maxv, 0 );


		sprintf( tmp, "\\out\\%04.04d_object%02.02d_gmm2.bmp", frame, obj_id );
		cvSaveImage( tmp, out );


		cvReleaseImage( &out );
		cvReleaseImage( &out2 );
	}
#endif


	free( points_x );
	free( points_y );
	points_x = 0;
	points_y = 0;


	// calculate thresholds for each component
	double thr[ Object::GMM_COMPONENTS ];

	for ( int m = 0; m < total_components; ++m )
	{
		Object::BivariateGaussian* gc = obj->gmm.getComponent( m );
		double w = obj->gmm.getWeight( m );

		// find principal axes of component
		CvPoint2D32f eigenvalues = gc->eigenvalues();
		CvPoint2D32f eigenvector1 = gc->eigenvector( eigenvalues.x );

		double mx = gc->getMean().x;
		double my = gc->getMean().y;

		CvPoint2D32f fv1 = cvPoint2D32f( mx + NUM_SIGMA * eigenvalues.x * eigenvector1.x,
										 my + NUM_SIGMA * eigenvalues.x * eigenvector1.y );

		thr[ m ] = w * gc->prob( fv1.x, fv1.y );
		if ( thr[ m ] < COMPONENT_PROB_THRESHOLD )
			thr[ m ]  = COMPONENT_PROB_THRESHOLD;
	}

	obj->gmm.setThresholds( thr );



	// create probability LUT map for this GMM
	obj->gmm.initMap( 360, 256 );
	CvRect gmm_roi = obj->gmm.getSigmaROI( 3.0 );

	for ( int s = 0; s < 256; ++s )
	{
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_map, obj->gmm.getMap(), s );

		for ( int h = 0; h < 360; ++h )
		{
			int x = HS_SPACE_RADIUS + s * cos( h * DEG_2_RAD ) / SPACE_SCALE;
			int y = HS_SPACE_RADIUS + s * sin( h * DEG_2_RAD ) / SPACE_SCALE;

			if ( x >= gmm_roi.x && x <= gmm_roi.x + gmm_roi.width &&
				 y >= gmm_roi.y && y <= gmm_roi.y + gmm_roi.height )
			{
				p_prob_map[ h ] = obj->gmm.probThr( x, y );
			}
			else
			{
				p_prob_map[ h ] = 0.0;
			}
		}
	}


#ifdef DUMP_EM_RESULTS
	{	
		IplImage* out = cvCreateImage( cvSize( 360, 256 ), IPL_DEPTH_8U, 1 );
		iplSet( out, 0 );

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvSetImageCOI( obj->gmm.getMap(), 1 );
		cvMinMaxLoc( obj->gmm.getMap(), &minv, &maxv, &minpt, &maxpt );
		cvSetImageCOI( obj->gmm.getMap(), 0 );
		cvConvertScale( obj->gmm.getMap(), out, 255.0 / maxv, 0 );

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_prob.bmp", frame, obj_id );
		cvSaveImage( tmp, out );

		cvReleaseImage( &out );
	}
#endif
}




void trackColourGMM( 	ObjectList* objects,
						int obj_id,
						IplImage* bkg_res,
						int foreground,
						IplImage* src,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl )
{
	assert( objects );
	assert( bkg_res );
	assert( bkg_res->nChannels == 1 );
	assert( bkg_res->depth == IPL_DEPTH_8U );
	assert( src );
	assert( src->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( src_hsv );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );


	Object* obj = objects->getAt( obj_id );
	assert( obj );


	IplImage* prob_res = cvCreateImage( cvSize( src->width, src->height ), IPL_DEPTH_32F, 1 );
	


	IplImage* map = obj->gmm.getMap();
	PRE_COMPUTE_IMAGE_ROW_PTRS( float, p_map, map );


	// determine a ROI where we need to calculate the probability
	int max_movement = CfgParameters::instance()->getMaxColourFrameMovement();

	CvRect bbox = obj->bounding_box;
	int w = MAX( MIN( max_movement / 2, obj->bounding_box.width / 4 ), max_movement );
	int h = MAX( MIN( max_movement / 2, obj->bounding_box.height / 4 ), max_movement );

	// if object has been lost in last frame, then we relax the ROI a bit to allow a wider search
	if ( obj->lost )
	{
		w *= 1.5;
		h *= 1.5;
	}

	bbox.x -= w;
	bbox.y -= h;
	bbox.width += w*2;
	bbox.height += h*2;
	if ( bbox.x < 0 )	bbox.x = 0;
	if ( bbox.y < 0 )	bbox.y = 0;
	if ( bbox.x + bbox.width >= src->width )
		bbox.width = src->width - bbox.x;
	if ( bbox.y + bbox.height >= src->height )
		bbox.height = src->height - bbox.y;

	
	float** p_azimuth_map = CfgParameters::instance()->getAzimuthMapPtrs();
	float** p_radial_map  = CfgParameters::instance()->getRadialMapPtrs();


	long total_pxls = 0;
	double total_p = 0.0;
	double new_cx = 0.0, new_cy = 0.0;


	// calculate probability of each pixel
	int y = bbox.height;
	do
	{
		int yy = bbox.y + y -1;

		COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, yy );
		COMPUTE_IMAGE_ROW_PTR( unsigned char, p_bkg_res, bkg_res, yy );
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_res, prob_res, yy );
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, yy );

		int x = bbox.width;
		do
		{
			int xx = bbox.x + x -1;

			// if a foreground pixel
			if ( p_bkg_res[ xx ] >= foreground )
			{
				register int x3 = xx * 3;

				int h = p_src_hsv[ x3    ];
				int s = p_src_hsv[ x3 +1 ];
				int v = p_src_hsv[ x3 +2 ];

				if ( v > LOW_HSV_THRESHOLD_V && s < HIGH_HSV_THRESHOLD_S )
				{
					double p = p_map[ s ][ h ];

					// if high probability
					if ( p )
					{
						p_prob_res[ xx ] = p;

						float l = p_prob_lbl[ xx ];
						if ( l == 0 )
						{
							p_prob_lbl[ xx ] = 2000 + obj_id;

							total_p += p;
							new_cx += xx * p;
							new_cy += yy * p;
						}
						else
						{
							p_prob_lbl[ xx ] = 4000 + obj_id;

							total_p += p * HALF_PROB;
							new_cx += xx * p * HALF_PROB;
							new_cy += yy * p * HALF_PROB;
						}


						++total_pxls;
					}
					else
					{
						p_prob_res[ xx ] = 0;
					}
				}
				else
				{
					p_prob_res[ xx ] = 0;
				}
			}
			else
			{
				p_prob_res[ xx ] = 0;
			}
		}
		while( --x );
	}
	while( --y );


	// probability centroid
	new_cx /= total_p;
	new_cy /= total_p;

	double new_c_az = p_azimuth_map[ lrintf( new_cy ) ][ lrintf( new_cx ) ];
	double new_c_el = p_radial_map[ lrintf( new_cy ) ][ lrintf( new_cx ) ];

	double var_x = 0.0, var_y = 0.0;
	double var_az = 0.0, var_el = 0.0;
	double prev_azimuth = -99999;
	int boundary_fix = 0;


	if ( total_pxls )
	{
		// determine variance for object
		int y = bbox.height;
		do
		{
			int yy = bbox.y + y -1;

			COMPUTE_IMAGE_ROW_PTR( float, p_prob_res, prob_res, yy );
			COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, yy );

			int x = bbox.width;
			do
			{
				int xx = bbox.x + x -1;


				// if pixel labelled as belonging to this object
				int l = p_prob_lbl[ xx ];
				if ( l == ( 2000 + obj_id ) )
				{
					double p = p_prob_res[ xx ];

					
					var_x += ( new_cx - xx ) * ( new_cx - xx ) * p;
					var_y += ( new_cy - yy ) * ( new_cy - yy ) * p;


					double az = p_azimuth_map[ yy ][ xx ];

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

					prev_azimuth = az;

					
					// distance of pixel from mirror centre
					double el = p_radial_map[ yy ][ xx ];


					var_az += ( new_c_az - az ) * ( new_c_az - az ) * p;
					var_el += ( new_c_el - el ) * ( new_c_el - el ) * p;
				}
				else if ( l == ( 4000 + obj_id ) )
				{
					double p = p_prob_res[ xx ];

					
					var_x += ( new_cx - xx ) * ( new_cx - xx ) * p * HALF_PROB;
					var_y += ( new_cy - yy ) * ( new_cy - yy ) * p * HALF_PROB;

					
					double az = p_azimuth_map[ yy ][ xx ];

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

					prev_azimuth = az;

					
					// distance of pixel from mirror centre
					double el = p_radial_map[ yy ][ xx ];


					var_az += ( new_c_az - az ) * ( new_c_az - az ) * p * HALF_PROB;
					var_el += ( new_c_el - el ) * ( new_c_el - el ) * p * HALF_PROB;
				}
			}
			while( --x );
		}
		while( --y );


		// probability variance
		var_x = sqrt( var_x / total_p );
		var_y = sqrt( var_y / total_p );

		var_az = sqrt( var_az / total_p );
		var_el = sqrt( var_el / total_p );
	}


	if ( total_pxls )
	{
		obj->lost = false;
		obj->lost_count = 0;


		// update object's state
		obj->area = total_pxls;
		if ( obj->area > obj->max_area )
			obj->max_area = obj->area;

		obj->old_centre = obj->centre;
		obj->centre.x = new_cx;
		obj->centre.y = new_cy;

		int max_search_window = CfgParameters::instance()->getMaxColourSearchWindow();

		double var_3x = var_x * NUM_SIGMA;
		double var_3y = var_y * NUM_SIGMA;
		var_3x = MIN( max_search_window, var_3x );
		var_3y = MIN( max_search_window, var_3y );
		obj->bounding_box = cvRect( new_cx - var_3x, 
									new_cy - var_3y,
									var_3x * 2, var_3y * 2 );

		obj->min_azimuth = new_c_az - var_az * NUM_SIGMA;
		obj->max_azimuth = new_c_az + var_az * NUM_SIGMA;
		if ( obj->max_azimuth < obj->min_azimuth )
			obj->max_azimuth += 360;


		obj->min_radius = new_c_el - var_el * NUM_SIGMA;
		obj->max_radius = new_c_el + var_el * NUM_SIGMA;
	}
	else
	{
		obj->lost = true;
		++obj->lost_count;

		// leave object's state unchanged
	}



#ifdef DUMP_OBJ_PROBABILITY_RESULT
	{	
		IplImage* out = cvCreateImage( cvSize( src->width, src->height ), IPL_DEPTH_8U, 1 );
		iplSet( out, 0 );

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvMinMaxLoc( prob_res, &minv, &maxv, &minpt, &maxpt );
		cvConvertScale( prob_res, out, 200.0 / maxv, 50 );
		cvThreshold( out, out, 50, 0, CV_THRESH_TOZERO );

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_prob_res.bmp", frame, obj_id );
		cvSaveImage( tmp, out );

		cvReleaseImage( &out );
	}
#endif


	cvReleaseImage( &prob_res );
}




void trackColourGMM( 	ObjectList* objects,
						IplImage* bkg_res,
						int foreground,
						IplImage* src,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl )
{
	assert( prob_lbl );


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		trackColourGMM( objects, o, bkg_res, foreground, src, src_hsv, frame, prob_lbl );
	}


	// final label map
	{	
		int y = prob_lbl->height;
		do
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_bkg_res, bkg_res, y-1 );
			COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, y-1 );

			int x = prob_lbl->width;
			do
			{
				if ( p_bkg_res[ x-1 ] >= foreground )
				{
					if ( p_prob_lbl[ x-1 ] == 0 )
					{
						p_prob_lbl[ x-1 ] = 99999;
					}
				}
			}
			while( --x );
		}
		while( --y );
	}
}




void updateColourGMM( 	ObjectList* objects,
						int obj_id,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl );



void removeCloseBlobs(	BlobList* blobs, 
						IplImage* lbl_map,
						IplImage* src_hsv,
						long frame )
{
	for ( int b = blobs->first(); b != -1; b = blobs->next( b ) )
	{
		Blob* blob = blobs->getAt( b );

		CvRect roi = blob->bounding_box;


		double cx = 0;
		double cy = 0;
		long count = 0;

		int y = roi.height;
		do
		{
			int yy = roi.y + y -1;

			COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, yy );

			int x = roi.width;
			do
			{
				int xx = roi.x + x -1;

				if ( p_lbl_map[ xx ] == blob->label )
				{
					cx += xx;
					cy += yy;
					++count;
				}
			}
			while( --x );
		}
		while( --y );

		cx /= count;
		cy /= count;

		blob->centre.x = cx;
		blob->centre.y = cy;



		int grow_box = CfgParameters::instance()->getNearbyColourThreshold();

		int w = MAX( grow_box, roi.width / 2 );
		int h = MAX( grow_box, roi.height / 2 );

		roi.x -= w;
		roi.y -= h;
		roi.width += w*2;
		roi.height += h*2;
		if ( roi.x < 0 )	roi.x = 0;
		if ( roi.y < 0 )	roi.y = 0;
		if ( roi.x + roi.width >= lbl_map->width )
			roi.width = lbl_map->width - roi.x;
		if ( roi.y + roi.height >= lbl_map->height )
			roi.height = lbl_map->height - roi.y;


		double nearest_d = 99999;
		int nearest_obj = -1;

		y = roi.height;
		do
		{
			int yy = roi.y + y -1;

			COMPUTE_IMAGE_ROW_PTR( float, p_lbl_map, lbl_map, yy );

			int x = roi.width;
			do
			{
				int xx = roi.x + x -1;

				float l = p_lbl_map[ xx ];
				if ( l >= 2000 && l < 99999 )
				{
					double d1 = blob->centre.x - xx;
					double d2 = blob->centre.y - yy;
					//double d = sqrt( d1 * d1 + d2 * d2 );
					double d = ( d1 * d1 + d2 * d2 );

					if ( d < nearest_d )
					{
						nearest_d = d;
						nearest_obj = l > 3999 ? l - 4000 : l - 2000;

						if ( nearest_d <= 1 )
							break;
					}
				}

			}
			while( --x );
			if ( nearest_d <= 1 )
				break;
		}
		while( --y );

		if ( nearest_obj != -1 )
		{
			CvConnectedComp component;
			CvScalar fillColour = cvScalar( 2000 + nearest_obj );	//MM!!
			cvFloodFill( lbl_map, blob->seed, fillColour, cvScalarAll(0), cvScalarAll(0), &component, 8 );
			assert( component.area = blob->area );

			blobs->remove( b );
		}
	}


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		if ( ! obj->lost )
			updateColourGMM( objects, o, src_hsv, frame, lbl_map );
	}
}




void updateObjectGMM( 	ObjectList* objects,
						IplImage* src,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl )
{
	CfgParameters* cfg = CfgParameters::instance();
	bool sudden_death = cfg->getSuddenDeathRule();


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		if ( obj->lost )
		{
			if ( sudden_death )
			{
				if ( obj->area < 500)
				{
					LOG( "SUDDEN DEATH!!  " << o )
					objects->remove( o );
				}
			}
			else
			{
				--obj->lives;
				--obj->lives;


				// delete an object if it has no lives left, i.e. has 
				// been lost for quite a while
				if ( obj->lives < 0 )
				{
					objects->remove( o );
					LOG( "GAME OVER!!  " << o );
				}
				// delete an object if it has got really small in size
				// and at some point it was much larger, i.e. it is not
				// just appearing (coming into view), and it has been
				// lost for some time
				else if ( obj->area < obj->max_area )
				{
					if ( obj->max_area > 2 * cfg->getMinColourArea() &&
						 obj->max_area > 3 * obj->area &&
						 obj->lost_count > 2 )
					{
						objects->remove( o );
						LOG( "OBJECT VANISHED!!  " << o );
					}
				}
				else if ( obj->area < cfg->getMinColourArea() )
				{
					objects->remove( o );
					LOG( "TINY OBJECT!!  " << o );
				}
			}
		}
		else
		{
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

		if ( ! sudden_death )
		{
			++obj->lives;
		}

	}



	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		if ( ! obj->lost )
		{
			if ( obj->bounding_box.width == 0 || obj->bounding_box.height == 0 )
				break;

			long t0 = frame - obj->born;
			long t1 = frame - obj->gmm_updated;
			if ( ( t0 > 5 && t0 <= 25 )
				 || t1 > cfg->getColourUpdateRate()
			)
			{
				// can we update the object?
				bool can_update = true;

				// bounding box overlap?
				for ( int o2 = objects->first(); o2 != -1; o2 = objects->next( o2 ) )
				{
					if ( o2 != o )
					{
						Object* obj2 = objects->getAt( o2 );
						CvRect r = intersectRect( obj->bounding_box, obj2->bounding_box );
						if ( r.width != 0 && r.height != 0 )
							can_update = false;
					}
				}


				if ( can_update )
				{
					// common pixels?
					long total_a = 0;
					long total_b = 0;
					int y = obj->bounding_box.height;
					do
					{
						int yy = obj->bounding_box.y + y -1;

						COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, yy );

						int x = obj->bounding_box.width;
						do
						{
							int xx = obj->bounding_box.x + x -1;

							int l = p_prob_lbl[ xx ];
							if ( l == 2000 + obj->label )
								++total_a;
							else if ( l == 4000 + obj->label )
								++total_b;
						}
						while( --x );
					}
					while( --y );

#ifdef INCREMENTAL_EM_UPDATE
					if ( total_a > cfg->getMinColourUpdateSize() && 
						 total_b < 0.02 * total_a )
					{
						updateColourGMM(	objects,
											obj->label,
											prob_lbl,
											src,
											src_hsv,
											frame );
						obj->gmm_updated = frame;											
					}

					// after every 50 incremental updates, we do a full
					// update to correct fro any drift due to accumulation
					// of errors in the colour model
					if ( obj->iem_count > 50 )
					{
						obj->iem_count = 0;
#endif
						if ( total_a > cfg->getMinColourUpdateSize() && 
							 total_b < 0.02 * total_a )
						{
							fitColourGMM( 	objects,
											obj->label,
											prob_lbl,
											src,
											src_hsv,
											frame );
							obj->gmm_updated = frame;

							// do only one update in one frame; others in the next.

							// TO DO: to be really fair, we should check which object 
							// has not been updated for the longest time.
							
							return;
						}
#ifdef INCREMENTAL_EM_UPDATE
					}
#endif
				}
			}
		}
	}



	double mirror_H = cfg->getMirrorFocalLen() * 2;


	for ( int o = objects->first(); o != -1; o = objects->next( o ) )
	{
		Object* obj = objects->getAt( o );

		for ( int o2 = objects->next( o ); o2 != -1; o2 = objects->next( o2 ) )
		{
			if ( o2 != o )
			{
				Object* obj2 = objects->getAt( o2 );


				// calculating intersection using elev-azimuth bounding box
				CvRect obj_box = cvRect( obj->min_radius, obj->min_azimuth, 
										 lrintf( obj->max_radius - obj->min_radius ), 
										 lrintf( obj->max_azimuth - obj->min_azimuth ) );
				CvRect obj2_box = cvRect( obj2->min_radius, obj2->min_azimuth, 
										 lrintf( obj2->max_radius - obj2->min_radius ), 
										 lrintf( obj2->max_azimuth - obj2->min_azimuth ) );

				// adjust in case of azimuth boundary cross-over
				if ( obj->max_azimuth > 360.0 && obj2->max_azimuth < 180.0 ) 
					obj2_box.y += 360;
				if ( obj2->max_azimuth > 360.0 && obj->max_azimuth < 180.0 ) 
					obj_box.y += 360;
				if ( obj->max_azimuth < 0.0 && obj2->max_azimuth > 180.0 ) 
					obj2_box.y -= 360;
				if ( obj2->max_azimuth < 0.0 && obj->max_azimuth > 180.0 ) 
					obj_box.y -= 360;
		
				// scale altitude and azimuth to arbitrary units to make them
				// equal (in scale) so that overlap ratio is meaningful
				float az_factor = mirror_H / 90.0;
				obj_box.y *= az_factor;
				obj_box.height *= az_factor;
				obj2_box.y *= az_factor;
				obj2_box.height *= az_factor;

				// calculate overlap area ratio
				CvRect overlap_box = intersectRect( obj_box, obj2_box );
				double overlap = 0;

				if ( overlap_box.width > 0 && overlap_box.height > 0 )
				{
					double overlap_area = overlap_box.width * overlap_box.height;
					double obj_area = obj_box.width * obj_box.height;
					double obj2_area = obj2_box.width * obj2_box.height;

					overlap = ( 2.0 * overlap_area ) / ( obj_area + obj2_area );
				}



				if ( overlap > 0.80 && obj->area > cfg->getMinColourArea() * 2 )
				{
					LOG( "OVERLAP!! " << o << " " << o2 << "   (" << overlap <<", " << obj->area << ", " << frame - obj2->born );
					if ( frame - obj2->born < 600 )
					{
						LOG( "OVERLAP!! " << o << " " << o2 << "   (" << overlap <<", " << obj->area );
						objects->remove( o2 );
					}
				}
			}
		}
	}
}







void updateColourGMM( 	ObjectList* objects,
						int obj_id,
						IplImage* src_hsv,
						long frame,
						IplImage* prob_lbl )
{
	assert( objects );
	assert( src_hsv );
	assert( src_hsv->nChannels == 3 );
	assert( src_hsv->depth == IPL_DEPTH_32S );


	Object* obj = objects->getAt( obj_id );
	assert( obj );


	IplImage* prob_res = cvCreateImage( cvSize( src_hsv->width, src_hsv->height ), IPL_DEPTH_32F, 1 );
	


	IplImage* map = obj->gmm.getMap();
	PRE_COMPUTE_IMAGE_ROW_PTRS( float, p_map, map );

	int grow_box = CfgParameters::instance()->getNearbyColourThreshold();


	// determine a ROI where we need to re-compute centroid and variance
	CvRect bbox = obj->bounding_box;
	bbox.x -= grow_box;
	bbox.y -= grow_box;
	bbox.width += grow_box * 2;
	bbox.height += grow_box * 2;
	if ( bbox.x < 0 )	bbox.x = 0;
	if ( bbox.y < 0 )	bbox.y = 0;
	if ( bbox.x + bbox.width >= src_hsv->width )
		bbox.width = src_hsv->width - bbox.x;
	if ( bbox.y + bbox.height >= src_hsv->height )
		bbox.height = src_hsv->height - bbox.y;

	long total_pxls = 0;
	double total_p = 0.0;
	double new_cx = 0.0, new_cy = 0.0;


	// calculate probability of each pixel
	int y = bbox.height;
	do
	{
		int yy = bbox.y + y -1;

		COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, yy );
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_res, prob_res, yy );
		COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, yy );

		int x = bbox.width;
		do
		{
			int xx = bbox.x + x -1;

			// if a foreground pixel
			int l = p_prob_lbl[ xx ];
			if ( l == 2000 + obj_id || l == 4000 + obj_id )
			{
				register int x3 = xx * 3;

				int h = p_src_hsv[ x3    ];
				int s = p_src_hsv[ x3 +1 ];

				double p = p_map[ s ][ h ];

				p_prob_res[ xx ] = p;


				if ( l == 2000 + obj_id )
				{
					++total_pxls;
					total_p += p;
					new_cx += xx * p;
					new_cy += yy * p;
				}
				else
				{
					++total_pxls;
					total_p += p * HALF_PROB;
					new_cx += xx * p * HALF_PROB;
					new_cy += yy * p * HALF_PROB;
				}
			}
			else
			{
				p_prob_res[ xx ] = 0;
			}
		}
		while( --x );
	}
	while( --y );


	if ( total_p == 0 ) 
	{
		obj->lost = true;
		++obj->lost_count;

		return;
	}

	
	// probability centroid
	new_cx /= total_p;
	new_cy /= total_p;


	double var_x = 0.0, var_y = 0.0;

	if ( total_pxls )
	{
		// determine variance for object
		int y = bbox.height;
		do
		{
			int yy = bbox.y + y -1;

			COMPUTE_IMAGE_ROW_PTR( float, p_prob_res, prob_res, yy );
			COMPUTE_IMAGE_ROW_PTR( float, p_prob_lbl, prob_lbl, yy );

			int x = bbox.width;
			do
			{
				int xx = bbox.x + x -1;

				double p = p_prob_res[ xx ];


				// if pixel labelled as belonging to this object
				int l = p_prob_lbl[ xx ];
				if ( l == ( 2000 + obj_id ) )
				{
					var_x += ( new_cx - xx ) * ( new_cx - xx ) * p;
					var_y += ( new_cy - yy ) * ( new_cy - yy ) * p;
				}
				else if ( l == ( 4000 + obj_id ) )
				{
					var_x += ( new_cx - xx ) * ( new_cx - xx ) * p * HALF_PROB;
					var_y += ( new_cy - yy ) * ( new_cy - yy ) * p * HALF_PROB;
				}
			}
			while( --x );
		}
		while( --y );


		// probability variance
		var_x = sqrt( var_x / total_p );
		var_y = sqrt( var_y / total_p );
	}


	assert( total_pxls );

	// re-update object's state
	obj->area = total_pxls;
	if ( obj->area > obj->max_area )
		obj->max_area = obj->area;

	obj->centre.x = new_cx;
	obj->centre.y = new_cy;

	int max_search_window = CfgParameters::instance()->getMaxColourSearchWindow();

	double var_3x = var_x * NUM_SIGMA;
	double var_3y = var_y * NUM_SIGMA;
	var_3x = MIN( max_search_window, var_3x );
	var_3y = MIN( max_search_window, var_3y );
	obj->bounding_box = cvRect( new_cx - var_3x, 
								new_cy - var_3y,
								var_3x * 2, var_3y * 2 );


#ifdef DUMP_OBJ_PROBABILITY_RESULT
	{	
		IplImage* out = cvCreateImage( cvSize( src_hsv->width, src_hsv->height ), IPL_DEPTH_8U, 1 );
		iplSet( out, 0 );

		double minv, maxv;
		CvPoint minpt, maxpt;
		cvMinMaxLoc( prob_res, &minv, &maxv, &minpt, &maxpt );
		cvConvertScale( prob_res, out, 200.0 / maxv, 50 );
		cvThreshold( out, out, 50, 0, CV_THRESH_TOZERO );

		char tmp[200];
		sprintf( tmp, "\\out\\%04.04d_object%02.02d_prob_res2.bmp", frame, obj_id );
		cvSaveImage( tmp, out );

		cvReleaseImage( &out );
	}
#endif

	cvReleaseImage( &prob_res );
}
