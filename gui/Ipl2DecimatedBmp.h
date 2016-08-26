
#ifndef IPL_2_DECIMATED_BMP_H
#define IPL_2_DECIMATED_BMP_H
#pragma once


#include "Ipl2Bmp.h"
#include <math.h>
#include <cv.h>



class Ipl2DecimatedBmp
{
private:

	IplImage* small_image;
	Ipl2Bmp converter;

	RECT r;
	int sx, sy;


public:

	Ipl2DecimatedBmp()
	{
		small_image = 0;
		r.right = r.bottom = r.top = r.left = 0;
	}



	~Ipl2DecimatedBmp()
	{
		reset();
	}



	void setOutputSize( RECT r )
	{
		this->r = r;
	}



	void update( const IplImage* image );



	//------- wrapper functions: forward to converter class -------------------

	int width()
	{
		return converter.width();
	}


	int height()
	{
		return converter.height();
	}


	bool empty()
	{
		return small_image == 0;
	}


	void reset()
	{
		converter.reset();

		if ( small_image != 0 )
		{
			cvReleaseImage( &small_image );
			small_image = 0;
		}
	}


	void paint( HDC dc, int offset_x = 0, int offset_y = 0 )
	{
		converter.paint( dc, offset_x, offset_y );
	}


	void paint( CStatic* ctrl )
	{
		converter.paint( ctrl );
	}

};




#endif
