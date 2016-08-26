
#ifndef IJL_IPL_HPP
#define IJL_IPL_HPP
#pragma once


#include <ipl.h>
#include <cv.h>



// reads a JPEG image file. If buffer is 0, an IplImage will be allocated
// and must be freed using iplDeallocate. If a buffer is specified, no
// allocation is done and the buffer is re-used.
extern IplImage* ReadJpeg(	LPCSTR filename, 
							bool abort = true, 
							IplImage* buffer = 0 );


// same as ReadJpeg, except that only a part of the JPEG image (a 
// region-of-interest) is read, giving better performance, if one is
// interested in only part of the image.
extern IplImage* ReadJpegROI(	LPCSTR filename, 
								CvRect roi, 
								bool abort = true, 
								IplImage* buffer = 0 );


// reads the header information of a JPEG image file.
// the information is returned in the header of an IplImage object that
// has no image data in it.
extern IplImage* ReadHeaderJpeg( LPCSTR filename );


// reads the header information of a region-of-interest of a JPEG image file.
// the information is returned in the header of an IplImage object that has
// no image data in it.
extern IplImage* ReadHeaderJpegROI( LPCSTR filename, 
									CvRect roi );


// writes an IplImage to a JPEG file.
extern bool WriteJpeg(	IplImage* aImage,
						LPCSTR  filename );



#endif


