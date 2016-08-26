
#ifndef MIXTURE_MODEL_MAP_H
#define MIXTURE_MODEL_MAP_H


#include <ipl.h>
#include <cv.h>
#include <assert.h>
#include "MixtureModel.h"




template< class VECTOR, class DENSITY, int MAX_DENSITIES = 15 >
class MixtureModelMap : public MixtureModel< VECTOR, DENSITY, MAX_DENSITIES >
{
	IplImage* map;


public:

	
	MixtureModelMap()
		: MixtureModel< VECTOR, DENSITY, MAX_DENSITIES >()
	{
		map = 0;
	}



	~MixtureModelMap()
	{
		clearMap();
	}



	void clearMap()
	{
		if ( map != 0 )
			cvReleaseImage( &map );
		map = 0;
	}



	void initMap( int w, int h )
	{
		clearMap();
		map = cvCreateImage( cvSize( w, h ), IPL_DEPTH_32F, 1 );
		assert( map );
		iplSetFP( map, 0.0 );
	}



	inline IplImage* getMap()
	{
		return map;
	}



	MixtureModelMap& operator= ( const MixtureModelMap& other )
	{
		MixtureModel< VECTOR, DENSITY, MAX_DENSITIES >::operator=( other );

		if ( other.map )
		{
			clearMap();
			initMap( other.map->width, other.map->height );

			iplCopy( map, other.map );
		}

		return *this;
	}

};




#endif

