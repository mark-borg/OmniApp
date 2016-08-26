
#ifndef EM_H
#define EM_H


#include <ipl.h>
#include <cv.h>




template< class VECTOR, class DENSITY, class MIXTURE >
class EM
{
	int num_comp;
	int num_pts;

	double**	p_cm_xn;
	double*		p_xn;
	double**	p_ym_xn;
	double*		alpha_new;
	double*		sum_p_ym_xn;
	VECTOR*		mean_new;
	VECTOR*		var_new;
	double*		covar_new;

	double		last_likelihood;
	double		diff;
	int			iteration;


public:


	EM( int num_comp, int num_pts )
	{
		assert( num_comp > 0 );
		assert( num_pts > 0 );

		this->num_comp = num_comp;
		this->num_pts = num_pts;

		typedef double*	double_ptr;

		p_cm_xn = new double_ptr[ num_comp ];
		for ( int m = 0; m < num_comp; ++m )
			p_cm_xn[ m ] = new double[ num_pts ];

		p_xn = new double[ num_pts ];

		p_ym_xn = new double_ptr[ num_comp ];
		for ( m = 0; m < num_comp; ++m )
			p_ym_xn[ m ] = new double[ num_pts ];

		alpha_new = new double[ num_comp ];

		sum_p_ym_xn = new double[ num_comp ];

		mean_new = new VECTOR[ num_comp ];

		var_new = new VECTOR[ num_comp ];

		covar_new = new double[ num_comp ];

		iteration = 0;
		last_likelihood = -9999;
	}



	void e_step( MIXTURE* gmm, double* data_x, double* data_y )
	{
		++iteration;

		gmm->arrProb( data_x, data_y, num_pts, p_cm_xn );

		int n = num_pts;
		do
		{
			double sum_p = 0.0;

			int m = num_comp;
			do
			{
				sum_p += p_cm_xn[ m-1 ][ n-1 ];
			}
			while( --m );

			p_xn[ n-1 ] = sum_p;
		}
		while( --n );


		CvMat PX;
		cvInitMatHeader( &PX, 1, num_pts, CV_64F, p_xn );

		int m = num_comp;
		do
		{
			CvMat PCX, PYX;
			cvInitMatHeader( &PCX, 1, num_pts, CV_64F, &p_cm_xn[ m-1 ][0] );
			cvInitMatHeader( &PYX, 1, num_pts, CV_64F, &p_ym_xn[ m-1 ][0] );

			cvDiv( &PCX, &PX, &PYX );

			sum_p_ym_xn[ m-1 ] = cvSum( &PYX ).val[0];
		}
		while( --m );
	}



	void m_step( MIXTURE* gmm, double* data_x, double* data_y )
	{
		int m = num_comp;
		do
		{
			// new weight
			double sum_p = sum_p_ym_xn[ m-1 ];
			alpha_new[ m-1 ] = sum_p / num_pts;


			// new mean
			double mean_new0 = 0.0;
			double mean_new1 = 0.0;

			int n = num_pts;
			double* ptr_p = &p_ym_xn[ m-1 ][ n-1 ];
			do
			{
				double p = *ptr_p;
				--ptr_p;
				
				mean_new0 += p * data_x[ n-1 ];
				mean_new1 += p * data_y[ n-1 ];
			}
			while( --n );

			mean_new0 = mean_new0 / sum_p;
			mean_new1 = mean_new1 / sum_p;
			mean_new[ m-1 ].x = mean_new0;
			mean_new[ m-1 ].y = mean_new1;


			
			// new covariance matrix
			double var_new0 = 0.0;
			double var_new1 = 0.0;
			double covar_new0 = 0.0;

			n = num_pts;
			ptr_p = &p_ym_xn[ m-1 ][ n-1 ];
			do
			{
				double p = *ptr_p;
				--ptr_p;

				double d0 = data_x[ n-1 ] - mean_new0;
				double d1 = data_y[ n-1 ] - mean_new1;

				register double p_d1 = p * d1;

				var_new0 += p * d0 * d0;
				var_new1 += p_d1 * d1;
				covar_new0 += p_d1 * d0;
			}
			while( --n );

			var_new0 = var_new0 / sum_p;
			var_new1 = var_new1 / sum_p;

			if ( var_new0 < MIN_VARIANCE )
				var_new0 = MIN_VARIANCE;
			if ( var_new1 < MIN_VARIANCE )
				var_new1 = MIN_VARIANCE;

			var_new[ m-1 ].x = var_new0;
			var_new[ m-1 ].y = var_new1;

			covar_new0 = covar_new0 / sum_p;
			covar_new[ m-1 ] = covar_new0;
		}
		while( --m );
	}



	void updateParams( MIXTURE* gmm )
	{
		for ( int m = 0; m < num_comp; ++m )
		{
			DENSITY* g = gmm->getComponent( m );

			VECTOR covar;
			covar.x = covar_new[ m ];
			covar.y = covar_new[ m ];		// symmetric matrix

			g->set(	mean_new[ m ], var_new[ m ], covar );
			gmm->updateWeight( m, alpha_new[ m ] );
		}
	}



	bool hasConverged( MIXTURE* gmm, double epsilon = 0.001 )
	{
		double lklh = logLikelihood();
		bool converged = fabs( lklh - last_likelihood ) < epsilon;

		// store liklihood so that we can test for convergence
		// in the next iteration.
		last_likelihood = lklh;
		
		return converged;
	}



	double logLikelihood()
	{
		double* log_p_xn = new double[ num_pts ];

		// calculate log of p_xn[n]
		CvMat M, N;
		cvInitMatHeader( &M, 1, num_pts, CV_64F, p_xn );
		cvInitMatHeader( &N, 1, num_pts, CV_64F, log_p_xn );
		cvLog( &M, &N );

		// log likelihood = sum of logs of probabilities p_xn
		double loglk = cvSum( &N ).val[0] /= num_pts;

		delete[] log_p_xn;

		return loglk;
	}



	int whichComponent( int data_pt_n )
	{
		double high_prob = 0.0;
		int best_m = -1;

		int m = num_comp;
		do
		{
			double p = p_ym_xn[ m-1 ][ data_pt_n ];
			
			if ( p > high_prob )
			{
				high_prob = p;
				best_m = m-1;
			}
		}
		while( --m );

		return best_m;
	}



	~EM()
	{
		for ( int m = 0; m < num_comp; ++m )
			delete[] p_cm_xn[ m ];
		delete[] p_cm_xn;

		delete[] p_xn;

		for ( int m = 0; m < num_comp; ++m )
			delete[] p_ym_xn[ m ];
		delete[] p_ym_xn;

		delete[] alpha_new;

		delete[] sum_p_ym_xn;

		delete[] mean_new;

		delete[] var_new;

		delete[] covar_new;
	}
};




#endif

