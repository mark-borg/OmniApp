
#include "CvUtils.h"




CvRect unionRect( const CvRect& r1, const CvRect& r2 )
{
	int x0 = MIN( r1.x, r2.x );
	int y0 = MIN( r1.y, r2.y );
	int x1 = MAX( r1.x + r1.width, r2.x + r2.width );
	int y1 = MAX( r1.y + r1.height, r2.y + r2.height );
	return cvRect(	x0, y0, x1 - x0, y1 - y0 );
}




CvRect intersectRect( const CvRect& r1, const CvRect& r2 )
{
	CvRect r;
	r.x = r.y = -1;
	r.height = r.width = 0;

	int y0 = MAX( r1.y, r2.y );
	int y1 = MIN( r1.y + r1.height, r2.y + r2.height );

	if ( y1 <= y0 )
		return r;

	int x0 = MAX( r1.x, r2.x );
	int x1 = MIN( r1.x + r1.width, r2.x + r2.width );

	if ( x1 <= x0 )
		return r;

	r.x = x0;
	r.y = y0;
	r.width = x1 - x0;
	r.height = y1 - y0;

	return r;

}




void cvMulS( CvMat* srcarr1, double scalar, CvMat* dstarr )
{
    int size = srcarr1->width * srcarr1->height;

    const double* src1data = (const double*)( srcarr1->data.ptr );
    double* dstdata = (double*)( dstarr->data.ptr );

    do
    {
        dstdata[ size-1 ] = src1data[ size-1 ] * scalar;
    }
    while( --size );
}



