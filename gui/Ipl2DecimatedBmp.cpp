
#include "Ipl2DecimatedBmp.h"
#include "..\tools\float_cast.h"
#include "..\tools\timer.h"
#include "..\core\log.h"




void Ipl2DecimatedBmp::update( const IplImage* image )
{
	if ( small_image == 0 )
	{
		assert( r.right );
		assert( r.bottom );
		assert( r.left == 0 && r.top == 0 );

		sx = lrintf( floor( image->width / static_cast<double>( r.right ) ) );
		sy = lrintf( floor( image->height / static_cast<double>(  r.bottom ) ) );

		sx = sy = max( sx, sy );		// to keep aspect ratio

		small_image = cvCreateImage( cvSize( image->width / sx, image->height / sy ), 
									 image->depth, image->nChannels );

		iplSet( small_image, 0 );
	}

//	iplDecimate( (IplImage*) image, small_image, 1, sx, 1, sy, IPL_INTER_NN );
	iplResize( (IplImage*) image, small_image, 1, sx, 1, sy, IPL_INTER_NN );


	//----------------
	// there's a bug in microsoft window's bitmapped static controls: if a bitmap
	// isn't placed at the top-left corner of the client area of the static control,
	// then Windows will use the value of the first pixel as the background colour
	// to fill the rest of the image. In order to avoid changes in background colour
	// whenever the first pixel of the image changes colour, we explicitly set the
	// first pixel to black.
	small_image->imageData[0] = 0;
	if ( small_image->nChannels == 3 )
	{
		small_image->imageData[1] = 0;
		small_image->imageData[2] = 0;
	}
	//--------------------------------------

	converter.update( small_image );
}



