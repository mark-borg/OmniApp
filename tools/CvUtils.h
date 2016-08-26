
#ifndef CV_UTILS_H
#define CV_UTILS_H


#include <cv.h>



#define PI					3.1415926535897932384626433832795
#define DEG_2_RAD			(1/180.0)*PI
#define RAD_2_DEG			(1/PI)*180.0


#define PRE_COMPUTE_IMAGE_ROW_PTRS( TYPE, VAR_NAME, IMAGE )		\
				TYPE* VAR_NAME[ 800 ];		\
				ASSERT( IMAGE->height < 800 );		\
				{	\
					for ( register int k = 0; k < IMAGE->height; ++k )	\
						VAR_NAME[ k ] = (TYPE*) (IMAGE->imageData + k * IMAGE->widthStep);	\
				}


#define COMPUTE_IMAGE_ROW_PTR( TYPE, VAR_NAME, IMAGE, Y )	\
				register TYPE* VAR_NAME = (TYPE*) ( IMAGE->imageData + (Y) * IMAGE->widthStep );



#define MULT_3x3_MATRIX( A, B, R )		\
				{	\
					R[0] = A[0]*B[0] + A[1]*B[3] + A[2]*B[6];	\
					R[1] = A[0]*B[1] + A[1]*B[4] + A[2]*B[7];	\
					R[2] = A[0]*B[2] + A[1]*B[5] + A[2]*B[8];	\
					R[3] = A[3]*B[0] + A[4]*B[3] + A[5]*B[6];	\
					R[4] = A[3]*B[1] + A[4]*B[4] + A[5]*B[7];	\
					R[5] = A[3]*B[2] + A[4]*B[5] + A[5]*B[8];	\
					R[6] = A[6]*B[0] + A[7]*B[3] + A[8]*B[6];	\
					R[7] = A[6]*B[1] + A[7]*B[4] + A[8]*B[7];	\
					R[8] = A[6]*B[2] + A[7]*B[5] + A[8]*B[8];	\
				}



//						 ~~~~~~~~~~~
#define SQR( A )		( (A) * (A) )
//							  |
//							(---)	



CvRect unionRect( const CvRect& r1, const CvRect& r2 );
CvRect intersectRect( const CvRect& r1, const CvRect& r2 );



void cvMulS( CvMat* srcarr1, double scalar, CvMat* dstarr );



#endif

