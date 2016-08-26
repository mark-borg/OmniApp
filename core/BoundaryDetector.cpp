
#include "BoundaryDetector.h"
#include <math.h>




bool HoughCircle(	IplImage* src, 
					float* centre_x, float* centre_y, float* circle_radius,
					float circle_threshold,		// = 0.2
					int r_start,				// = -1
					int r_end )					// = -1
{
	// input must be a grayscale edge-map
	assert( src );
	assert( src->nChannels == 1 );
	assert( src->depth == IPL_DEPTH_8U );

	// determine number of levels in image pyramid
	int pyramid_levels = 1;

	int ht = src->height;
	while( ht > 50 )
	{
		ht = ht / 2;
		++pyramid_levels;
	}


	typedef IplImage* IplImagePtr;
	IplImagePtr* pyramid;
	pyramid = new IplImagePtr[ pyramid_levels ];
	assert( pyramid );


	// create the pyramid
	pyramid[ 0 ] = src;
	for ( int k = 1; k < pyramid_levels; ++k )
	{
		int f = (int) pow( 2.0, k );
		pyramid[ k ] = cvCreateImage( 
							cvSize( src->width / f, src->height / f ),
							IPL_DEPTH_8U, 1 );
		cvPyrDown( pyramid[ k-1 ], pyramid[ k ] );
	}


	// starting range values
	int level = pyramid_levels-1;

	int a_min = 0;
	int a_max = pyramid[ level ]->width -1;

	int b_min = 0;
	int b_max = pyramid[ level ]->height -1;
	
	int r_min = r_start != -1
					? (int)( r_start / pow( 2.0, pyramid_levels ) )
					: 1;
	int r_max = r_end != -1 
					? (int)( r_end / pow( 2.0, pyramid_levels ) ) 
					: (int) sqrt( (double) a_max * a_max + b_max * b_max );


	int peak_r = 0;
	float peak_a = 0;
	float peak_b = 0;
	bool ok = true;

	for ( level = pyramid_levels -1; level >= 0 && ok; --level )
	{
		// circle eqn:  (x-a)*(x-a) + (y-b)*(y-b) = r*r
		// circle centre = (a,b)  circle radius = r

		int a_range = a_max - a_min + 1;
		int b_range = b_max - b_min + 1;
		int r_range = r_max - r_min + 1;


		// create hough space
		IplImagePtr* hs;
		hs = new IplImagePtr[ r_range ];
		assert( hs );

		for ( int r = 0; r < r_range; ++r )
		{
			hs[ r ] = cvCreateImage( cvSize( a_range, b_range ), IPL_DEPTH_32F, 1 );
			iplSetFP( hs[ r ], 0.0 );
		}


		for ( int y = 0; y < pyramid[ level ]->height; ++y )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, ptr, pyramid[ level ], y );

			for ( int x = 0; x < pyramid[ level ]->width; ++x )
				if ( ptr[ x ] > 0 )
				{
					// for each radius...
					for ( int r = 0; r < r_range; ++r )
					{
						int radius = r_min + r;

						for ( int b = 0; b < b_range; ++b )
						{
							double q =	radius * radius - 
										( y - ( b + b_min ) ) * ( y - ( b + b_min ) );
							if ( q > 0 )
							{
								COMPUTE_IMAGE_ROW_PTR( float, hs_ptr, (hs[ r ]), b );
			
								double p = sqrt( q );
								
								int a = (int)( x - p ) - a_min;
								if ( a >= 0 && a < a_range )
									hs_ptr[ a ] = hs_ptr[ a ] + 1;

								a = (int)( x + p ) - a_min;
								if ( a >= 0 && a < a_range )
									hs_ptr[ a ] = hs_ptr[ a ] + 1;
							}
						}
					}
				}
		}


		//  now traverse hough space to find largest number of votes 
		long largest_votes = -1;
		peak_r = 0;

		for ( int r = 0; r < r_range; ++r )
		{
			float minv, maxv;
			iplMinMaxFP( hs[ r ], &minv, &maxv );

			int radius = r_min + r;
			int threshold = (int)( ( 2 * PI * radius ) * circle_threshold ); 

			if ( maxv >= threshold )
			{
				if ( maxv > largest_votes )
				{
					largest_votes = (long) maxv;
					peak_r = radius;
				}
			}
		}


		ok = ( peak_r != 0 );

		if ( ok )
		{
			int r = peak_r - r_min;

			// now determine centre point (from centroid in a-b plane)
			peak_a = 0;
			peak_b = 0;
			long peak_n = 0;

			for ( int b = 0; b < b_range; b++ )
			{
				COMPUTE_IMAGE_ROW_PTR( float, ptr, hs[ r ], b );
				for ( int a = 0; a < a_range; ++a )
					if ( ptr[ a ] >= largest_votes )
					{
						peak_a += a;
						peak_b += b;
						++peak_n;
					}
			}

			peak_a = peak_a / peak_n + a_min;
			peak_b = peak_b / peak_n + b_min;


			// reduce ranges for next iteration
			int reduction = ( level > pyramid_levels / 2 ) ? 2 : 4;

			a_min = (int)( 2 * peak_a - a_range / reduction );
			a_max = (int)( 2 * peak_a + a_range / reduction );
			b_min = (int)( 2 * peak_b - b_range / reduction );
			b_max = (int)( 2 * peak_b + b_range / reduction );
			
			r_min = (int)( 2 * peak_r - r_range / reduction );
			r_max = (int)( 2 * peak_r + r_range / reduction ); 

			if ( r_min == r_max )
			{
				r_min -= 3;
				r_max += 3;
			}

			if ( a_min == a_max )
			{
				a_min -= 5;
				a_max += 5;
			}

			if ( b_min == b_max )
			{
				b_min -= 5;
				b_max += 5;
			}


			// clean hough space
			for ( r = 0; r < r_range; ++r )
				cvReleaseImage( &hs[ r ] );
		}
	}


	// pyramid cleanup
	for ( int k = 1; k < pyramid_levels; ++k )
		cvReleaseImage( &pyramid[ k ] ); 
	delete[] pyramid;


	// return values
	*centre_x = peak_a;
	*centre_y = peak_b;
	*circle_radius = (float) peak_r;

	return ok;
}


