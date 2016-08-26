
#include "IterativeThreshold.h"




bool fnIterativeThreshold( IplImage* src, IplImage* dst, 
						   int* threshold,
						   int initial_threshold )	// initial threshold = 128
{
	assert( src );
	assert( src->nChannels == 1 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( initial_threshold >= 0 && initial_threshold < 256 );

	int T = initial_threshold;
	int oldT = -1;

	if ( T == 0 )			// not to have an empty background set
		T = 1;

	int bailout = 100;


#define FAST_VERSION
#ifdef FAST_VERSION

	long histogram[ 256 ];
	memset( histogram, 0, 256 * sizeof( long ) );

	// build histogram of image
	for ( int y = 0; y < src->height; ++y )
	{
		COMPUTE_IMAGE_ROW_PTR( unsigned char, ptr, src, y );

		for ( int x = 0; x < src->width; ++x )
			++histogram[ ptr[ x ] ];
	}

#endif


	do 
	{
		oldT = T;
		--bailout;

		long bkg = 0, frg = 0;
		long nbkg = 0, nfrg = 0;

#ifdef FAST_VERSION
		for ( int k = 0; k < 256; ++k )
		{
			if ( k < T )
			{
				bkg += k * histogram[ k ];
				nbkg += histogram[ k ];
			}
			else
			{
				frg += k * histogram[ k ];
				nfrg += histogram[ k ];
			}			
		}
#else
		for ( int y = 0; y < src->height; ++y )
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, ptr, src, y );

			for ( int x = 0; x < src->width; ++x )
			{
				register unsigned char v = ptr[ x ];

				if ( v < T )
				{
					bkg += v;
					nbkg++;
				}
				else
				{
					frg += v;
					nfrg++;
				}
			}
		}
#endif

		long abkg = bkg / nbkg;
		long afrg = frg / nfrg;
		
		T = (unsigned char)( ( abkg + afrg ) / 2 );
	}
	while( T != oldT && bailout != 0 );

	iplThreshold( src, dst, T );
	*threshold = T;

	return bailout != 0;
}


