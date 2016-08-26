
#ifndef MIXTURE_MODEL_H
#define MIXTURE_MODEL_H


#include <iostream>
#include <ipl.h>
#include <cv.h>

using namespace std;




template< class VECTOR, class DENSITY, int MAX_DENSITIES = 15 >
class MixtureModel
{
protected:

	DENSITY components[ MAX_DENSITIES ];
	float	weights[ MAX_DENSITIES ];
	float	thresholds[ MAX_DENSITIES ];

	unsigned char num;



public:


	MixtureModel()
	{
		num = 0;
	}



	int numComponents()
	{
		return num;
	}



	void reset()
	{
		num = 0;
	}



	void setThresholds( double* thr )
	{
		assert( thr );

		for ( int i = 0; i < num; ++i )
			thresholds[ i ] = thr[ i ];
	}



	CvRect getSigmaROI( float sigmas = 3.0 )
	{
		assert( num );
		CvRect roi = components[ 0 ].getSigmaROI( sigmas );

		for ( int i = 1; i < num; ++i )
		{
			CvRect roi2 = components[ i ].getSigmaROI( sigmas );
			roi = unionRect( roi, roi2 );
		}	

		return roi;
	}

	

	void addComponent( const DENSITY& c, double weight )
	{
		assert( num < MAX_DENSITIES );
		components[ num ]	= c;
		weights[ num++ ]	= weight;
	}



	inline DENSITY* getComponent( int i )
	{
		assert( i >= 0 && i < num );
		
		return &( components[ i ] );
	}



	inline double getWeight( int i )
	{
		assert( i >= 0 && i < num );

		return weights[ i ];
	}



	void updateWeight( int i, double w )
	{
		assert( i >= 0 && i < num );
		
		weights[ i ] = w;
	}



	inline void arrProb( double* data_x, double* data_y, int num_pts, double** p_cm_xn )
	{
		int i = num;
		do
		{
			components[ i-1 ].arrProb( data_x, data_y, num_pts, &p_cm_xn[ i-1 ][ 0 ], weights[ i-1 ] );
		}
		while( --i );
	}



	double probThr( double vx, double vy )
	{
		double total = 0.0;

		for ( int i = 0; i < num; ++i )
		{
			double p = components[ i ].prob( vx, vy );
			if ( p > thresholds[ i ] )
				total += weights[ i ] * p;
		}

		assert( total >= 0.0 && total <= 1.0 );
		return total;
	}



	struct SortClump
	{
		double  key;
		DENSITY element;
		int		original_order;
	};



	void sort( int* sort_order = 0 )
	{
		SortClump* sc = new SortClump[ num ];

		for ( int i = 0; i < num; ++i )
		{
			sc[ i ].key				= weights[ i ];
			sc[ i ].element			= components[ i ];
			sc[ i ].original_order	= i;
		}

		qsort( sc, num, sizeof( SortClump ), compare );

		for ( i = 0; i < num; ++i )
		{
			weights[ i ]		= sc[ i ].key;
			components[ i ]		= sc[ i ].element;
			if ( sort_order != 0 )
				sort_order[ i ] = sc[ i ].original_order;
		}

		delete[] sc;
	}



	void dump( ostream& out = cout )
	{
		for ( int i = 0; i < num; ++i )
		{
			out << "# " << i 
				 << " : w=" << weights[ i ] << " ";
			components[ i ].dump( out );
			out << endl; 
		}
		out << endl;
	}

};




#endif

