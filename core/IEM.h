
#ifndef IEM_H
#define IEM_H


#include <ipl.h>
#include <cv.h>




template< class VECTOR, class DENSITY, class MIXTURE, int NUM_COMP >
class IEM
{
	int num_pts;

	double W[ NUM_COMP ];
	CvPoint2D64d C[ NUM_COMP ];
	double R[ NUM_COMP ][4];
	
	CvPoint2D64d M[ NUM_COMP ];		// arrays for stroing the sufficient statistics
	double Q[ NUM_COMP ][4];
	double N[ NUM_COMP ];
	double p[ NUM_COMP ];
	double x[ NUM_COMP ];

	double L, lambda, min_weight;


public:


	IEM( int num_pts )
	{
		assert( num_pts > 0 );

		this->num_pts = num_pts;

		typedef double*	double_ptr;

		lambda = 0.01;
		min_weight = 0.01;
	}



	void update( MIXTURE* gmm, double* data_x, double* data_y )
	{
		for ( int i = 0; i < 1; ++i )
		{
			L = 0;

			for ( int m = 0; m < NUM_COMP; ++m )
			{
				M[ m ] = C[ m ];


				Q[ m ][0] = C[ m ].x * C[ m ].x;
				Q[ m ][1] = C[ m ].x * C[ m ].y;
				Q[ m ][2] = C[ m ].y * C[ m ].x;
				Q[ m ][3] = C[ m ].y * C[ m ].y;


				N[ m ] = 1;
			}


			for ( int n = 0; n < num_pts; ++n )
			{
				// E Step
				{	
					double sum_p = 0.0;

					for ( int m = 0; m < NUM_COMP; ++m )
					{
						BivariateGaussian g;
						g.set(	C[ m ],
								FeatureVector( sqrt( R[ m ][0] ), sqrt( R[ m ][3] ) ),
								FeatureVector( R[ m ][1], R[ m ][2] ) );
						
						p[ m ] = W[ m ] * g.prob( points[ n ] );
						sum_p += p[ m ];
					}

					double largest_p = 0.0;
					int largest = -1;
					for ( m = 0; m < NUM_COMP; ++m )
					{
						x[ m ] = p[ m ] / sum_p;
						
						if ( p[ m ] > largest_p )
						{
							largest_p = p[ m ];
							largest = m;
						}
					}

					if ( largest != -1 )
					{
						m = largest;

						M[ m ].x = M[ m ].x + points[ n ].pt.x;
						M[ m ].y = M[ m ].y + points[ n ].pt.y;

						Q[ m ][0] = Q[ m ][0] + points[ n ].pt.x * points[ n ].pt.x;
						Q[ m ][1] = Q[ m ][1] + points[ n ].pt.x * points[ n ].pt.y;
						Q[ m ][2] = Q[ m ][2] + points[ n ].pt.y * points[ n ].pt.x;
						Q[ m ][3] = Q[ m ][3] + points[ n ].pt.y * points[ n ].pt.y;

						N[ m ]++;

						L += log( sum_p ) / num_pts;
					}
				}


				// M Step
				const int lmt = 1000;		// 2 dim x 2 clusters
				if ( ( n % lmt == ( lmt - 1 ) ) || ( n == ( num_pts - 1 ) ) )
				{
					cout << "\nM Step!" << endl;

					double sum_n = 0;

					for ( int m = 0; m < NUM_COMP; ++m )
					{
						sum_n += N[ m ];
					}


					for ( m = 0; m < NUM_COMP; ++m )
					{
						C[ m ].x = M[ m ].x / N[ m ];
						C[ m ].y = M[ m ].y / N[ m ];


						R[ m ][0] = fabs( Q[ m ][0] / N[ m ] - 
									( M[ m ].x * M[ m ].x ) / ( N[ m ] * N[ m ] ) 
									+ lambda );
						R[ m ][1] = Q[ m ][1] / N[ m ] - 
									( M[ m ].x * M[ m ].y ) / ( N[ m ] * N[ m ] ) 
									+ 0;
						R[ m ][2] = Q[ m ][2] / N[ m ] - 
									( M[ m ].y * M[ m ].x ) / ( N[ m ] * N[ m ] ) 
									+ 0;
						R[ m ][3] = fabs( Q[ m ][3] / N[ m ] - 
									( M[ m ].y * M[ m ].y ) / ( N[ m ] * N[ m ] ) 
									+ lambda );


						W[ m ] = N[ m ] / sum_n;
					}


#ifdef SPLIT_CLUSTERS
					// split clusters
					if ( n != ( num_pts - 1 ) )
					{
						double largest_weight = 0.0;
						double smallest_weight = 1.0;
						int largest = -1;
						int smallest = -1;
						for ( m = 0; m < NUM_COMP; ++m )
						{
							if ( W[ m ] > largest_weight )
							{
								largest_weight = W[ m ];
								largest = m;
							}
							if ( W[ m ] < smallest_weight )
							{
								smallest_weight = W[ m ];
								smallest = m;
							}
						}

						if ( smallest_weight < min_weight )
						{
							C[ smallest ].x = C[ largest ].x + sqrt( R[ largest ][0] );
							C[ smallest ].y	= C[ largest ].y + sqrt( R[ largest ][3] );

							C[ largest ].x -= sqrt( R[ largest ][0] );
							C[ largest ].y -= sqrt( R[ largest ][3] );
						}
					}
#endif
				}
			}
		}
	}
};




#endif

