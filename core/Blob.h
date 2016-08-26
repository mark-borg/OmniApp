
#ifndef BLOB_H
#define BLOB_H


#include <ipl.h>
#include <cv.h>



class Blob
{
public:

	int				label;
	CvPoint			seed;
	CvPoint			centre;
	CvRect			bounding_box;
	float			area;


	Blob();

};




#endif

