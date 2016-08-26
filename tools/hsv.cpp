
#include "hsv.h"
#include "cvutils.h"
#include <math.h>




void rgb2hsv( unsigned char r, 
			  unsigned char g,
			  unsigned char b,
			  int& hue,					// in range: [0..360]
			  int& sat,					// in range: [0..255]
			  int& val )				// in range: [0..255]
{
	int m = r;
	m = g > m ? g : m;
	m = b > m ? b : m;

	int n = r;
	n = g < n ? g : n;
	n = b < n ? b : n;

	val = m;
	sat = ( m != 0 ) ? ( ( 255 * ( m - n ) / m ) ) : 0;

	hue = 0;
	if ( sat != 0.0 )
	{
		int d = m - n;
		if ( r == m )
			hue = ( 60 * ( g - b ) ) / d;
		else if ( g == m )
			hue = 120 + ( 60 * ( b - r ) ) / d;
		else if ( b == m )
			hue = 240 + ( 60 * ( r - g ) ) / d;

		if ( hue < 0.0 )
			hue += 360;
	}
}




void convertImageRGB2HSV( IplImage* src, IplImage* hsv, IplImage* mask /* =0 */ )
{
	assert( src );
	assert( hsv );
	assert( src->nChannels == 3 );
	assert( hsv->nChannels == 3 );
	assert( src->depth == IPL_DEPTH_8U );
	assert( hsv->depth == IPL_DEPTH_32S );


	bool use_mask = mask != 0;

	if ( use_mask )
	{
		int y = src->height;
		do
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y-1 );
			COMPUTE_IMAGE_ROW_PTR( int, p_hsv, hsv, y-1 );
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );

			int x = src->width;
			do
			{
				if ( p_msk[ x-1 ] )
				{
					int x3 = (x-1)*3;

					unsigned char b = p_src[ x3 ];
					unsigned char g = p_src[ x3 +1 ];
					unsigned char r = p_src[ x3 +2 ];

					int h, s, v;
					rgb2hsv( r, g, b, h, s, v );

					p_hsv[ x3    ] = h;
					p_hsv[ x3 +1 ] = s;
					p_hsv[ x3 +2 ] = v;
				}
			}
			while( --x );
		}
		while( --y );
	}
	else
	{
		int y = src->height;
		do
		{
			COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y-1 );
			COMPUTE_IMAGE_ROW_PTR( int, p_hsv, hsv, y-1 );

			int x = src->width;
			do
			{
				int x3 = (x-1)*3;

				unsigned char b = p_src[ x3 ];
				unsigned char g = p_src[ x3 +1 ];
				unsigned char r = p_src[ x3 +2 ];

				int h, s, v;
				rgb2hsv( r, g, b, h, s, v );

				p_hsv[ x3    ] = h;
				p_hsv[ x3 +1 ] = s;
				p_hsv[ x3 +2 ] = v;
			}
			while( --x );
		}
		while( --y );
	}
}




void hsv2rgb( int hue,					// in range: [0..360]
			  int sat,					// in range: [0..255]
			  int val,					// in range: [0..255]
			  unsigned char& r,			// in range: [0..255]
			  unsigned char& g,
			  unsigned char& b )
{
	// note that this function has not been optimised for speed yet.
	// that is, it's still using floating-point, unlike rgb2hsv().
	// this function is not used within any time-critical part of 
	// the program, yet. when there is a need for its use, then
	// the function will be changed to use integer arithmetic.

	double rr, gg, bb;

	if ( sat == 0 )
	{
		rr = 1;
		gg = 1;
		bb = 1;
	}
	else
	{
		if ( hue == 360 )
			hue = 0;

		double h = hue / 60.0;			// h is now in range [0..6)
		double v = val / 255.0;			// v is now in range [0..1]
		double s = sat / 255.0;			// s is now in range [0..1]

		int i = (int) floor( h );
		double f = h - i;
		double p = v * ( 1.0 - s );
		double q = v * ( 1.0 - ( s * f ) );
		double t = v * ( 1.0 - ( s * ( 1.0 - f ) ) );

		switch( i )
		{
		case 0:	rr = v; 
				gg = t;
				bb = p;
				break;

		case 1:	rr = q; 
				gg = v;
				bb = p;
				break;

		case 2:	rr = p; 
				gg = v;
				bb = t;
				break;

		case 3:	rr = p; 
				gg = q;
				bb = v;
				break;

		case 4:	rr = t; 
				gg = p;
				bb = v;
				break;

		case 5:	rr = v; 
				gg = p;
				bb = q;
				break;
		}
	}

	r = (unsigned char) ( rr * 255 );
	g = (unsigned char) ( gg * 255 );
	b = (unsigned char) ( bb * 255 );
}



