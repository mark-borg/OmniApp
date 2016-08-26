
#include "DistinctColours.h"
#include <assert.h>



#define MAX_COLOURS	14




DistinctColours::DistinctColours()
{
	colours = new CvScalar[ MAX_COLOURS ];
	assert( colours );

	colours[ 0 ] = CV_RGB(		255,	255,	255		);
	colours[ 1 ] = CV_RGB(		0,		0,		255		);
	colours[ 2 ] = CV_RGB(		0,		255,	0		);
	colours[ 3 ] = CV_RGB(		255,	0,		0		);
	colours[ 4 ] = CV_RGB(		255,	0,		255		);
	colours[ 5 ] = CV_RGB(		0,		255,	255		);
	colours[ 6 ] = CV_RGB(		255,	255,	0		);
	colours[ 7 ] = CV_RGB(		0,		0,		180		);
	colours[ 8 ] = CV_RGB(		0,		180,	0		);
	colours[ 9 ] = CV_RGB(		180,	0,		0		);
	colours[ 10 ] = CV_RGB(		180,	0,		180		);
	colours[ 11 ] = CV_RGB(		0,		180,	180		);
	colours[ 12 ] = CV_RGB(		180,	180,	0		);
	colours[ 13 ] = CV_RGB(		180,	180,	180		);

	used = 0;
}




DistinctColours::~DistinctColours()
{
	delete colours;
}




CvScalar DistinctColours::get()
{
	return colours[ used ];
}




CvScalar DistinctColours::next()
{
	++used;
	if ( used == MAX_COLOURS )
		used = 0;

	return get();
}


