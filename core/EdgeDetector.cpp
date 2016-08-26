
#include "EdgeDetector.h"
#include <highgui.h>




void SimplifyExternalContours( IplImage* img )
{
	assert( img );

	CvMemStorage* storage = 0;
	storage = cvCreateMemStorage( 1000 );

	CvSeq* contour = 0;
	int header_size = sizeof CvContour;

	cvFindContours( img, storage, &contour, header_size, 
					CV_RETR_EXTERNAL,
					CV_CHAIN_APPROX_SIMPLE );

	iplSet( img, 0 );

	while( contour != 0 )
	{
		if ( CV_IS_SEQ_CURVE( contour ) )
	        cvDrawContours( img, contour, CV_RGB(255,255,255), CV_RGB(255,255,255), 0 );
	
		// traverse next contour;
		contour = contour->h_next;
	}

	cvReleaseMemStorage( &storage );
}


