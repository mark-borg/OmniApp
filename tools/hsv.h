
#ifndef HSV_H
#define HSV_H


#include <ipl.h>



// hue is unstable for v < LOW_HSV_THRESHOLD_V 
// and for s > HIGH_HSV_THRESHOLD_S.
#define	LOW_HSV_THRESHOLD_V		12	// approx max of 5 degree error in hue (or 1%)
#define	HIGH_HSV_THRESHOLD_S	246	// approx max of 5 degree error in hue (or 1%)




void rgb2hsv( unsigned char r, 
			  unsigned char g,
			  unsigned char b,
			  int& hue,					// in range: [0..360]
			  int& sat,					// in range: [0..255]
			  int& val );				// in range: [0..255]


void convertImageRGB2HSV( IplImage* src, IplImage* hsv, IplImage* mask = 0 );


void hsv2rgb( int hue,					// in range: [0..360]
			  int sat,					// in range: [0..255]
			  int val,					// in range: [0..255]
			  unsigned char& r,			// in range: [0..255]
			  unsigned char& g,
			  unsigned char& b );



#endif

