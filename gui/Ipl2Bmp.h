
#ifndef IPL_2_BMP_H
#define IPL_2_BMP_H
#pragma once



#define WIN32_LEAN_AND_MEAN		// exclude rarely-used stuff from Windows headers
#include <afxwin.h>
#include <ipl.h>
#include <cv.h>
#include <assert.h>
#include "..\tools\ThreadSafeHighgui.h"
#include "..\tools\timer.h"
#include "..\core\log.h"



class Ipl2Bmp
{
private:

	HDC memdc;
	HBITMAP hbmp;

	unsigned char* bmp_info_buffer;
	BITMAPINFO* bmp_info;
	BITMAPINFOHEADER* bmp_info_header;

	void* image_data;					// raw image data


public:

	Ipl2Bmp()
	{
		// create a memory device context
		memdc = CreateCompatibleDC( 0 );			
		assert( memdc );


		// create underlying buffer for BITMAPINFO = size of header + space for palette
		bmp_info_buffer = new unsigned char[ sizeof( BITMAPINFO ) + 255 * sizeof( RGBQUAD ) ];
		assert( bmp_info_buffer );
		bmp_info = (BITMAPINFO*) bmp_info_buffer;
		bmp_info_header = &( bmp_info->bmiHeader );


		image_data = 0;
	}



	~Ipl2Bmp()
	{
		deallocate();

		delete[] bmp_info_buffer;
		DeleteDC( memdc );
	}

	
	
	int width()
	{
		return bmp_info_header->biWidth;
	}



	int height()
	{
		return bmp_info_header->biHeight;
	}



	bool empty()
	{
		return image_data == 0;
	}



	void reset()
	{
		deallocate();
	}



	void paint( HDC dc, int offset_x = 0, int offset_y = 0 )
	{
		BitBlt( dc, offset_x, offset_y, 
				bmp_info_header->biWidth, bmp_info_header->biHeight, 
				memdc, 0, 0, SRCCOPY );
	}



	void paint( CStatic* ctrl )
	{
		HDC hDC;
		hDC = ::GetDC( NULL );
		HBITMAP hbmp = CreateDIBitmap(	hDC,
										bmp_info_header, 
										CBM_INIT,
										image_data,
										bmp_info,
										DIB_RGB_COLORS );

		HBITMAP old_bmp = ctrl->SetBitmap( hbmp );
		
		if ( old_bmp )
			DeleteObject( old_bmp );

		ReleaseDC( NULL, hDC );
	}



	void update( const IplImage* image )
	{
		if ( image == 0 )
			return;


		if ( image_data == 0 || 
			 bmp_info_header->biWidth != image->width ||
			 bmp_info_header->biHeight != image->height ||
			 bmp_info_header->biBitCount != ( image->nChannels * 8 ) )
		{
			if ( image_data )
				deallocate();


			memset( bmp_info_header, 0, sizeof( *bmp_info_header ) );
			bmp_info_header->biSize			= sizeof( BITMAPINFOHEADER ); 
			bmp_info_header->biWidth		= image->width;
			bmp_info_header->biHeight		= image->height;
			bmp_info_header->biBitCount		= image->nChannels * 8;
			bmp_info_header->biCompression	= BI_RGB;		// no compression
			bmp_info_header->biPlanes		= 1;
			bmp_info_header->biClrUsed		= 0;
			bmp_info_header->biClrImportant	= 0;
			bmp_info_header->biSizeImage	= 0;	// unused for biCompression = BI_RGB

			if( image->nChannels == 1 )
			{
				RGBQUAD* palette = bmp_info->bmiColors;
				for( int i = 0; i < 256; i++ )
				{
					palette[ i ].rgbBlue = palette[ i ].rgbGreen = palette[ i ].rgbRed = (unsigned char) i;
					palette[ i ].rgbReserved = 0;
				}
			}


			// create the bitmap and raw image data
			hbmp = CreateDIBSection( memdc, bmp_info, DIB_RGB_COLORS, &image_data, 0, 0 );
			assert( image_data );

			// activate the bitmap in the memory device context
			SelectObject( memdc, hbmp );
		}

		SIZE size;
		size.cx = image->width;
		size.cy = image->height;


		// convert the IPL image into the raw image data buffer
		CvMat dst;
		cvInitMatHeader( &dst, size.cy, size.cx, 
						 image->nChannels == 3 ? CV_8UC3 : CV_8UC1,
						 image_data, ( size.cx * image->nChannels + 3) & -4 );
		safe_highgui.Lock();
		{
			cvConvertImage( image, &dst, image->origin == 0 );
		}
		safe_highgui.Unlock();
	}



	IplImage* reConvert()
	{
		IplImage* ipl = iplCreateImageHeader(	3,				// channels
												0,				// no alpha
												IPL_DEPTH_8U,
												"RGB",			// colour model
												"BGR",			// colour ordering
												IPL_DATA_ORDER_PIXEL,
												IPL_ORIGIN_TL,
												IPL_ALIGN_QWORD,
												bmp_info_header->biWidth,
												bmp_info_header->biHeight,
												0, 0, 0, 0 );

		iplConvertFromDIBSep( bmp_info_header, (const char*) image_data, ipl );
		return ipl;
	}



protected:

	void deallocate()
	{
		// previous image?
		if ( image_data != 0 )
		{
			DeleteObject( hbmp );
			image_data = 0;
		}
	}

};




#endif
