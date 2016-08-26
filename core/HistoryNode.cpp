
#include "HistoryNode.h"
#include "CfgParameters.h"
#include "Log.h"
#include "..\tools\float_cast.h"
#include "..\tools\ThreadSafeHighgui.h"
#include <process.h>
#include <direct.h>



#define ESTIMATE_3D_POS



bool prepareOutputDir()
{
	// remove any existing files
	int rc = _rmdir( ".\\out" );
	if ( rc != 0 )
		system( "rd .\\out /s/q" );		// this is slower and briefly opens a screen console

	rc = _mkdir( ".\\out" );
	if ( rc != 0 )
	{
		CLOG( "History Node: error creating output directory!!", 0x000000ff );
		CLOG( "Either no permission or directory is already in use!!", 0x000000ff );
		CLOG( "History node is aborting!!", 0x000000ff );

		return false;
	}

	return true;
}




// for now, these are defined as static with fixed-length arrays.
// this can be amended to allow the lists to grow dynamically and
// make them more generic.
static CvPoint last_pos[ 50 ];
static int last_max_area[ 50 ];
static int total_age[ 50 ];
static int last_checked = 0;
static bool first_object = true;




bool process( IplImage* img, ObjectList* objects, long frame )
{
	if ( objects->size() == 0 )
		return true;

	char tmp[ 100 ];


#ifdef ESTIMATE_3D_POS
	//---------------------------------------------------------------
	// ESTIMATE 3D POSITION
	{
		float** p_azimuth_map = CfgParameters::instance()->getAzimuthMapPtrs();
		float** p_radial_map  = CfgParameters::instance()->getRadialMapPtrs();
		int mx = CfgParameters::instance()->getMirrorCentreX();
		int my = CfgParameters::instance()->getMirrorCentreY();

		IplImage* out = iplCloneImage( img );
		iplSet( out, 0 );

		for ( int o = objects->first(); o != -1; o = objects->next( o ) )
		{
			Object* obj = objects->getAt( o );
			
			if ( obj->path )
			{
				int h = obj->path->first();
				if ( h != -1 )
				{
					CvPoint2D32f *pos2, *pos1 = obj->path->getAt( h );

					for ( ; h < obj->path->getNextFreeIndex(); ++h )
					{
						pos2 = obj->path->getAt( h );
						
						double az1 = p_azimuth_map[ lrintf( pos1->y ) ][ lrintf( pos1->x ) ] + 180;
						double al1 = p_radial_map[ lrintf( pos1->y ) ][ lrintf( pos1->x ) ];

						double az2 = p_azimuth_map[ lrintf( pos2->y ) ][ lrintf( pos2->x ) ] + 180;
						double al2 = p_radial_map[ lrintf( pos2->y ) ][ lrintf( pos2->x ) ];

						double pd1 = tan( ( 90.0 - al1 ) * DEG_2_RAD ) * 40;
						double z1 = sin( az1 * DEG_2_RAD ) * pd1 + my;
						double x1 = cos( az1 * DEG_2_RAD ) * pd1 + mx;

						double pd2 = tan( ( 90.0 - al2 ) * DEG_2_RAD ) * 40;
						double z2 = sin( az2 * DEG_2_RAD ) * pd2 + my;
						double x2 = cos( az2 * DEG_2_RAD ) * pd2 + mx;

						cvLine( out, cvPoint( x1, z1 ), 
									 cvPoint( x2, z2 ),
									 CV_RGB(200,200,200) );
						pos1 = pos2;
					}
				}
			}
		}

		//save output
		//cvSaveImage( "d:\\a.bmp", out );

		iplDeallocate( out, IPL_IMAGE_ALL );
	}
#endif

	

	//---------------------------------------------------------------
	// SAVE HISTORY TO HTML FILE 
	for ( int o = last_checked; o < objects->getNextFreeIndex(); ++o )
	{
		if ( objects->isFree( o ) )
		{
			// object has died!
			// add its last entry to object's history file
			if ( last_pos[ o ].x != -1 )
			{
				total_age[ o ] = frame - total_age[ o ];

				sprintf( tmp, ".\\out\\%d\\index.htm", o );
				FILE* html = fopen( tmp, "a" );

				fprintf( html, "<tr><td>#%05.05d</td>", frame );
				fprintf( html, "<td></td>\n", frame );
				fprintf( html, "<td>Object no longer visible - record has been deleted." );
				if ( total_age[ o ] < 150 )
				{
					fprintf( html, "<p style=\"color: rgb(255, 0, 0);\">Short-lived object, may be noise." );
				}
				fprintf( html, "</td></tr>" );

				fclose( html );

				last_pos[ o ] = cvPoint( -1, -1 );

				if ( last_checked == o )
					++last_checked;
			}
		}
		else
		{
			Object* obj = objects->getAt( o );

			if ( ! obj->lost )
			{
				long age = frame - obj->born;

				CvRect box = obj->bounding_box;
				int ix = MAX( MIN( box.width, 25 ), 100 );
				int iy = MAX( MIN( box.height, 25 ), 100 );
				box.width += ix;
				box.height += iy;
				box.x -= ix / 2;
				box.y -= iy / 2;


				if ( age == 0 )
				{
					sprintf( tmp, ".\\out\\%d", o );
					_mkdir( tmp );

					last_max_area[ o ] = 0;
					total_age[ o ] = obj->born;
				}

				
				double d0 = obj->centre.x - last_pos[ o ].x;
				double d1 = obj->centre.y - last_pos[ o ].y;
				double d = sqrt( d0 * d0 + d1 * d1 );


				// save information
				if ( age == 0 ||
					 d > 75 )
				{
					// save snapshot of object
					sprintf( tmp, ".\\out\\%d\\%d.bmp", o, frame );

					cvSetImageROI( img, box );
					safe_highgui.Lock();
					{
						cvSaveImage( tmp, img );
					}
					safe_highgui.Unlock();

					if ( obj->area > last_max_area[ o ] )
					{
						sprintf( tmp, ".\\out\\%d\\main.bmp", o, frame );
						safe_highgui.Lock();
						{
							cvSaveImage( tmp, img );
						}
						safe_highgui.Unlock();
					}

					cvResetImageROI( img );


					// add entry to object's history file
					sprintf( tmp, ".\\out\\%d\\index.htm", o );
					FILE* html = fopen( tmp, "a" );

					if ( age == 0 )
					{
						fprintf( html, "<html>\n<body>\n" );
						fprintf( html, "<h2 style=\"color:rgb(51,51,255)\">Object %d</h2><p><p>\n", o );
						fprintf( html, "view trajectory map: <a href=\"./map.htm\"></a><p><hr><p><p>\n" );
						fprintf( html, "<table border=2>\n" );
						fprintf( html, "<tr><td>Frame Number</td><td>Photo</td><td>Comments</td>\n" );
					}

					fprintf( html, "<tr><td>#%05.05d</td><td>", frame );
					fprintf( html, "<img src=\"./%d.bmp\"></img>\n", frame );
					fprintf( html, "</td><td>" );
					if ( age == 0 )
					{
						fprintf( html, "Object first detected." );
					}
					fprintf( html, "</td></tr>" );

					fclose( html );


					// if first time, add an entry to main history file
					if ( age == 0 )
					{
						sprintf( tmp, ".\\out\\index.htm" );
						FILE* main = fopen( tmp, "a" );

						if ( first_object )
						{
							fprintf( main, "<html>\n<body>\n" );
							fprintf( main, "<h2 style=\"color:rgb(51,51,255)\">List of Objects</h2><p><p>" );
							fprintf( main, "Video Stream = %s<p><hr><p>", CfgParameters::instance()->getImageSource() );
							fprintf( main, "<table border=2 style=\"width:50%%\" cellpadding=5>\n" );
							fprintf( main, "<tr><td>Object ID #</td><td>Photo</td><td></td>\n" );
							
							first_object = false;
						}

						fprintf( main, "<tr><td># %d</td>", o );
						fprintf( main, "<td><img src=\"./%d/main.bmp\"></img></td>", o );
						fprintf( main, "<td><a href=\"./%d/index.htm\">view history</a></td></tr>\n", o );

						fclose( main );
					}


					// save object's info for next round
					last_pos[ o ].x = lrintf( obj->centre.x );
					last_pos[ o ].y = lrintf( obj->centre.y );
					last_max_area[ o ] = obj->max_area;
				}


			}
		}
	}

	return true;
}



