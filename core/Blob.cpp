

#include "Blob.h"




Blob::Blob()
{
	label = -1;
	seed = cvPoint( -1, -1 );
	bounding_box = cvRect( -1, -1, 0, 0 );
	area = 0;
}


