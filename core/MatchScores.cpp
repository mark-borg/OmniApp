
#include "BlobTracker.h"
#include "MatchScores.h"




float MatchScores::OVERLAP_WEIGHT	= 1.0f;
float MatchScores::DISTANCE_WEIGHT	= 1.0f;
float MatchScores::COLOUR_WEIGHT	= 2.0f;
float MatchScores::AREA_WEIGHT		= 0.5f;


float MatchScores::TOTAL_WEIGHT		
			=	MatchScores::OVERLAP_WEIGHT +
				MatchScores::DISTANCE_WEIGHT +
				MatchScores::COLOUR_WEIGHT +
				MatchScores::AREA_WEIGHT;




MatchScores::MatchScores( int max_objects, int max_clusters )
{
	typedef double*		double_ptr;

	
	this->max_objects = max_objects;
	this->max_clusters = max_clusters;


	overlap		= new double_ptr[ max_clusters ];
	distance	= new double_ptr[ max_clusters ];
	colour		= new double_ptr[ max_clusters ];
	area		= new double_ptr[ max_clusters ];
	total		= new double_ptr[ max_clusters ];

	for ( int k = 0; k < max_clusters; ++k )
	{
		overlap[ k ]	= new double[ max_objects ];
		distance[ k ]	= new double[ max_objects ];
		colour[ k ]		= new double[ max_objects ];
		area[ k ]		= new double[ max_objects ];
		total[ k ]		= new double[ max_objects ];

		for ( int o = 0; o < max_objects; ++o )
		{
			overlap[ k ][ o ]	= 0.0;
			distance[ k ][ o ]	= 0.0;
			colour[ k ][ o ]	= 0.0;
			area[ k ][ o ]		= 0.0;			
			total[ k ][ o ]		= 0.0;
		}
	}
}




MatchScores::~MatchScores()
{
	for ( int k = 0; k < max_clusters; ++k )
	{
		delete[] overlap[ k ];
		delete[] distance[ k ];
		delete[] colour[ k ];
		delete[] area[ k ];
		delete[] total[ k ];
	}

	delete[] overlap;
	delete[] distance;
	delete[] colour;
	delete[] area;
	delete[] total;
}




void MatchScores::sumTotalScore( ObjectList* objects )
{
	for ( int c = 0; c < max_clusters; ++c )
		for ( int o = 0; o < max_objects; ++o )
		{
			Object* obj = objects->getAt( o );

			if ( obj )
			{
				total[ c ][ o ] =	
					OVERLAP_WEIGHT	* overlap[ c ][ o ] +
					DISTANCE_WEIGHT	* distance[ c ][ o ] + 
					COLOUR_WEIGHT	* colour[ c ][ o ] +
					AREA_WEIGHT		* area[ c ][ o ];
			}
		}
}
