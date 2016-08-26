
#ifndef BIVARIATE_GAUSSIAN_H
#define BIVARIATE_GAUSSIAN_H


#include <iostream>
#include <ipl.h>
#include <cv.h>
#include "..\tools\CvUtils.h"

using namespace std;



#define MIN_VARIANCE				36
#define PROBABILITY_ZERO_CUTOFF		4.0	




template< class VECTOR >
class BivariateGaussian
{
	VECTOR mean;			// mean [m0 m1]
	VECTOR var;				// variance [sd0*sd0 sd1*sd1]
	float covar;			// covariance [cv01 cv01]	// note matrix is symmetric
	
	float covar_det;		// determinant of covariance matrix [sd0*sd0	cv01	]
							//									[cv01		sd1*sd1	]
	float norm_factor;		// normalisation factor: 1 / ( 2 * PI * SQRT( covar_det ) )

	unsigned char xmin, xmax;		// resolution for these variables is reduced by 4
	unsigned char ymin, ymax;


public:


	const VECTOR& getMean() const
	{
		return mean;
	}



	const VECTOR& getVar() const
	{
		return var;
	}



	const VECTOR& getCovar() const 
	{
		VECTOR v;
		v.x = covar;
		v.y = covar;
		return v;
	}



	void set( const VECTOR& mean, 
			  const VECTOR& var, 
			  const VECTOR& covar )
	{
		this->mean = mean;
		this->var  = var;

		assert( covar.x == covar.y );
		this->covar = covar.x;

		if ( this->var.x < MIN_VARIANCE )
			this->var.x = MIN_VARIANCE;
		if ( this->var.y < MIN_VARIANCE )
			this->var.y = MIN_VARIANCE;


		// pre-calculations
		covar_det = this->var.x * this->var.y - this->covar * this->covar;

		norm_factor  = 1.0 / ( 2.0 * PI * sqrt( covar_det ) );

		
		// beyond this region, probabilities are returned as zero
		getSigmaROI( PROBABILITY_ZERO_CUTOFF );
	}



	CvRect getSigmaROI( float sigmas = 3.0 )
	{
		// roughly calculate bounding box 
		VECTOR e = eigenvalues();
		double k = MAX( sigmas * e.x, 3 * e.y );
		
		xmin = lrintf( mean.x - k - 0.5 ) >> 2;
		xmax = lrintf( mean.x + k + 0.5 ) >> 2;
		ymin = lrintf( mean.y - k - 0.5 ) >> 2;
		ymax = lrintf( mean.y + k + 0.5 ) >> 2;

		return cvRect( xmin << 2, ymin << 2, 
					   ( xmax - xmin +1 ) << 2, ( ymax - ymin +1 ) << 2 );
	}



	double prob( double vx, double vy )
	{
		register double p = 0.0;

		int vrx = lrintf( vx ) >> 2, 
			vry = lrintf( vy ) >> 2;

		if ( vrx > xmin && vrx < xmax && vry > ymin && vry < ymax )
		{
			register double d0, d1;
			d0 = vx - mean.x;
			d1 = vy - mean.y;

			register double d0d1 = d0 * d1;

			if ( covar_det > 0.001 )
			{
				p = norm_factor * exp( -1.0 / ( 2.0 * covar_det ) * 
													(	d0 * d0 * var.x - 
														2.0 * d0d1 * covar + 
														d1 * d1 * var.y ) );
			}

			assert( p >= 0.0 && p <= 1.0 );
		}

		return p;
	}



	void arrProb( double* data_x, double* data_y, int num_pts, double* result, double weight = 1.0 )
	{
		CvMat R, X, Y;
		cvInitMatHeader( &X, 1, num_pts, CV_64F, data_x );
		cvInitMatHeader( &Y, 1, num_pts, CV_64F, data_y );
		cvInitMatHeader( &R, 1, num_pts, CV_64F, result );

		if ( covar_det < 0.001 )
		{
			cvSetZero( &R );
		}
		else
		{
			double* dx = new double[ num_pts ];
			double* dy = new double[ num_pts ];
			double* tmpA = new double[ num_pts ];
			double* tmpB = new double[ num_pts ];
		
			CvMat DX, DY, TMPA, TMPB;
			cvInitMatHeader( &DX, 1, num_pts, CV_64F, dx );
			cvInitMatHeader( &DY, 1, num_pts, CV_64F, dy );
			cvInitMatHeader( &TMPA, 1, num_pts, CV_64F, tmpA );
			cvInitMatHeader( &TMPB, 1, num_pts, CV_64F, tmpB );

			cvSubS( &X, cvRealScalar( mean.x ), &DX );
			cvSubS( &Y, cvRealScalar( mean.y ), &DY );

			float norm_factor2 = -1.0 / ( 2.0 * covar_det );

			// tmpA = d0 * d0 * var.x
			cvMul( &DX, &DX, &TMPA, norm_factor2 * var.x );

			// tmpB = d1 * d1 * var.y
			cvMul( &DY, &DY, &TMPB, norm_factor2 * var.y );

			// tmpA = tmpA + tmpB
			cvAdd( &TMPA, &TMPB, &TMPA );

			// tmpB = d0 * d1 * covar
			cvMul( &DX, &DY, &TMPB, 2.0 * covar * norm_factor2 );

			// tmpA = tmpA - tmpB
			cvSub( &TMPA, &TMPB, &TMPA );

			// tmpB = exp( tmpA )
			cvExp( &TMPA, &TMPB );

			// R = norm_factor * tmpB
			cvMulS( &TMPB, norm_factor * weight, &R );

			delete[] dx;
			delete[] dy;
			delete[] tmpA;
			delete[] tmpB;
		}
	}



	VECTOR eigenvalues()
	{
		double tmp = var.x - var.y;
		tmp = sqrt( tmp * tmp + 4.0 * covar * covar );

		double sum_var = var.x + var.y;

		VECTOR eigenvalue;
		eigenvalue.y = sqrt( ( sum_var + tmp ) / 2.0 );
		eigenvalue.x = sqrt( ( sum_var - tmp ) / 2.0 );

		return eigenvalue;
	}



	VECTOR eigenvector( double eigenvalue )
	{
		double tmp = eigenvalue * eigenvalue - var.x;
		double denom = sqrt( tmp * tmp + covar * covar );

		VECTOR eigenvector;
		eigenvector.x = tmp / denom;
		eigenvector.y = covar / denom;

		return eigenvector;
	}



	void dump( ostream& out = cout )
	{
		out << "mean=" << mean.x << "," << mean.y;
		out << " var=" << var.x << "," << var.y;
		out << " covar=" << covar << "," << covar;
	}

};




#endif

