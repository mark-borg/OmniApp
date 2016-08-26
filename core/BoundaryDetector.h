
#ifndef BOUNDARY_DETECTOR_H
#define BOUNDARY_DETECTOR_H


#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"




bool HoughCircle(	IplImage* src, 
					float* centre_x, float* centre_y, float* circle_radius,
					float circle_threshold = 0.2,
					int r_start = -1, 
					int r_end = -1 );



template< class ALLOCATOR >
class BoundaryDetector
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;
	bool* result;


	IplData wrk1;



public:


	BoundaryDetector( string my_name, bool* detector_result, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;
		result = detector_result;
	}



	~BoundaryDetector()
	{
		if ( wrk1.img != 0 )
			allocator->release( wrk1.img );
	}



	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		// get sources
		IplData* buffer = & source_buffers[ 0 ];
		assert( buffer );
		assert( buffer->img );

		IplData* output = & source_buffers[ 1 ];
		bool display = output && output->img;

		
		// acquire result image buffers
		if ( display )
		{
			assert( buffer->img->width <= output->img->width &&
					buffer->img->height <= output->img->height );
			assert( output->img->depth == IPL_DEPTH_8U );

			IplImageResource img_typ;
			img_typ = IplImageResource( output->img->width, output->img->height, 
										IPL_DEPTH_8U, output->img->nChannels );

			assert( wrk1.img == 0 );
			wrk1.img = allocator->acquire( img_typ );
			assert( wrk1.img );
		}


		// some extra validation on the incoming image
		assert( buffer->img->nChannels == 1 );
		assert( buffer->img->depth == IPL_DEPTH_8U );


		// detect mirror boundary
		float posx, posy, radius;
		bool ok = HoughCircle( buffer->img, 
					 &posx, &posy, &radius, 0.05f, 
					 buffer->img->width/4);


		CfgParameters* params = CfgParameters::instance();


		// update mirror parameters
		if ( ok )
		{
			const int allowance = 8;	// JPEG box size

			int xa = posx - radius - allowance;
			int ya = posy - radius - allowance;
			int xb = xa + radius * 2 + 2 * allowance;
			int yb = ya + radius * 2 + 2 * allowance;
			xa = xa < 0 ? 0 : xa;
			ya = ya < 0 ? 0 : ya;
			xb = xb > buffer->img->width ? buffer->img->width : xb;
			yb = yb > buffer->img->height ? buffer->img->height : yb;
			

			// update parameters
			params->getMirrorCentreX() = posx - xa +1;
			params->getMirrorCentreY() = posy - ya +1;
			params->getMirrorBoundary() = radius;
			
			double aspect_x = params->getAspectRatioX();
			double aspect_y = params->getAspectRatioY();

			params->getRoiMinX() = xa * aspect_x;
			params->getRoiMaxX() = xb * aspect_x;
			params->getRoiMinY() = ya * aspect_y;
			params->getRoiMaxY() = yb * aspect_y;


			// display something for the user to keep him interested
			if ( display )
			{
				const CvScalar red = CV_RGB(0,0,255);		//MM!
				const CvScalar red2 = CV_RGB(0,0,128);		//MM!

				iplCopy( output->img, wrk1.img );


				// show detection zone
				double l = params->getMirrorLowerBound();
				double u = params->getMirrorUpperBound();
				double H = params->getMirrorFocalLen() * 2.0;

				cvCircle( wrk1.img, cvPoint( posx, posy ), l, red2, 1 );
				cvCircle( wrk1.img, cvPoint( posx, posy ), u, red2, 1 );


				// show mirror boundary and mirror parameter H
				cvCircle( wrk1.img, cvPoint( posx, posy ), radius, red, 2 );
				cvCircle( wrk1.img, cvPoint( posx, posy ), H, red, 1 );


				// show centre
				cvLine( wrk1.img,
						cvPoint( posx - 10, posy ),
						cvPoint( posx + 10, posy ),
						red, 2 );
				cvLine( wrk1.img,
						cvPoint( posx, posy - 10 ),
						cvPoint( posx, posy + 10 ),
						red, 2 );


				// show ROI
				cvRectangle( wrk1.img, 
							 cvPoint( xa, ya ),
							 cvPoint( xb, yb ),
							 red, 1 );
			}
		}
		else
		{
			// detector failed - fill with default values

			params->getMirrorCentreX() = buffer->img->width / 2;	// centre of image
			params->getMirrorCentreY() = buffer->img->height / 2;
			params->getMirrorBoundary() = MIN( buffer->img->width / 2, buffer->img->height / 2 );
			
			params->getRoiMinX() = 0;
			params->getRoiMaxX() = buffer->img->width -1;
			params->getRoiMinY() = 0;
			params->getRoiMaxY() = buffer->img->height -1;

			
			// we don't have anyhting to display
			display = false;
		}


		// inform caller of success of detector or not
		*result = ok;


		// move result to sink buffers
		for ( int i = 0; i < sink_buffers.size(); ++i )
			sink_buffers[ i ] = display ? wrk1 : *buffer;
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( wrk1.img != 0 )
		{
			allocator->release( wrk1.img );
			wrk1.img = 0;
		}
	}

};



#endif
