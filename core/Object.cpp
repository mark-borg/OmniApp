
#include "Object.h"



#define INITIAL_PATH_LIST_SIZE	50




Object::Object()
{
	label			= -1;
	centre			= cvPoint2D32f( -1, -1 );
	bounding_box	= cvRect( -1, -1, 0, 0 );
	area			= 0;

	first_pos		= cvPoint2D32f( -1, -1 );
	max_area		= 0;

	min_radius		= 999999;
	max_radius		= -1;
	min_azimuth		= 9999;
	max_azimuth		= -1;

	old_centre		= cvPoint2D32f( -1, -1 );
	old_bounding_box = cvRect( -1, -1, 0, 0 );

	born			= -1;
	lives			= 0;
	lost_count		= 0;
	lost			= false;
	merged			= false;
	occluded		= false;
	was_last_merged = false;

	for ( int k = 0; k < H_NUM_BINS; ++k )
		h_bins[ k ] = 0;
	for ( int k = 0; k < S_NUM_BINS; ++k )
		s_bins[ k ] = 0;
	for ( int k = 0; k < V_NUM_BINS; ++k )
		v_bins[ k ] = 0;
	histogram_empty = true;

	gmm_updated = -1;

	path = 0;
}




void Object::initHistory()
{
	assert( path == 0 );

	path = new LWList< CvPoint2D32f >( INITIAL_PATH_LIST_SIZE, INITIAL_PATH_LIST_SIZE );
	assert( path );
}




void Object::clearHistory()
{
	if ( path != 0 )
		delete path;
	
	path = 0;
}




Object::~Object()
{
	clearHistory();
}




Object& Object::operator= ( const Object& other )
{
	label			= other.label;
	centre			= other.centre;
	bounding_box	= other.bounding_box;
	area			= other.area;
	first_pos		= other.first_pos;
	max_area		= other.max_area;

	
	for ( int k = 0; k < H_NUM_BINS; ++k )
		h_bins[ k ] = other.h_bins[ k ];
	for ( int k = 0; k < S_NUM_BINS; ++k )
		s_bins[ k ] = other.s_bins[ k ];
	for ( int k = 0; k < V_NUM_BINS; ++k )
		v_bins[ k ] = other.v_bins[ k ];

	histogram_empty	= other.histogram_empty;

	gmm				= other.gmm;
	gmm_updated		= other.gmm_updated;


	min_radius		= other.min_radius;
	max_radius		= other.max_radius;
	min_azimuth		= other.min_azimuth;
	max_azimuth		= other.max_azimuth;

	old_centre		= other.old_centre;
	old_bounding_box = other.old_bounding_box;

	born			= other.born;	
	lives			= other.lives;	
	lost_count		= other.lost_count;	
	lost			= other.lost;
	merged			= other.merged;
	occluded		= other.occluded;
	was_last_merged	= other.was_last_merged;


	if ( other.path )
	{
		clearHistory();
		initHistory();

		for ( int p = 0; p < other.path->capacity(); ++p )
		{
			if ( ! other.path->isFree( p ) )
			{
				float x = other.path->getAt( p )->x;
				float y = other.path->getAt( p )->y;

				path->add( cvPoint2D32f( x, y ) );
			}
		}
	}


	return *this;
}

