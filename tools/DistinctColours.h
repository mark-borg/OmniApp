
#ifndef DISTINCT_COLOURS_H
#define DISTINCT_COLOURS_H


#include <cv.h>


class DistinctColours
{
	CvScalar* colours;

	int used;


public:

	DistinctColours();


	~DistinctColours();


	CvScalar get();


	CvScalar next();

};



#endif

