
#define VC_EXTRALEAN			// Exclude rarely-used stuff from Windows headers
#include <afxwin.h>			// for AfxMessageBox(...)
#include <stdio.h>
#include <ijl.h>
#include "ijl_ipl.h"




// forward declarations of local functions
IplImage* CreateImageHeaderFromIJL( const JPEG_CORE_PROPERTIES* jcprops,
									IplTileInfo* tileInfo );

IplImage* CreateImageHeaderFromIJL( const JPEG_CORE_PROPERTIES* jcprops,
									IplTileInfo* tileInfo,
									CvRect roi );

BOOL CreateImageHeaderFromIJL(	const JPEG_CORE_PROPERTIES* jcprops,
								IplTileInfo* tileInfo,
								IplImage* image );

BOOL CreateImageHeaderFromIJL(	const JPEG_CORE_PROPERTIES* jcprops,
								IplTileInfo* tileInfo,
								CvRect roi,
								IplImage* image );

bool SetJPEGProperties( JPEG_CORE_PROPERTIES& jcprops,
						const IplImage*  image );




////////////////////////////////////////////////////////////////////////

IplImage* ReadJpeg( LPCSTR filename, bool abort, IplImage* aImage )
{
	BOOL error = FALSE;
	BOOL pre_allocated = (aImage != 0);
	IJLERR  jerr;
	IplImage* out = 0;

	// allocate the JPEG core properties
	JPEG_CORE_PROPERTIES jcprops;

	__try
	{
		jerr = ijlInit(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't initialize IJLib...
			error = TRUE;
			__leave;
		}

		jcprops.JPGFile = const_cast<LPSTR>(filename);

		// IJL supports two different DCTs:
		// the default is fast and accurate.
		// the second (IJL_AAN) is faster but less accurate.
		// Enabling the second is done by the following line:
		// jcprops.jprops.dcttype = IJL_AAN;

		// read image parameters: width, height,...
		jerr = ijlRead(&jcprops,IJL_JFILE_READPARAMS);
		if(IJL_OK != jerr)
		{
			// can't read JPEG image parameters...
			error = TRUE;
			__leave;
		}


		// create the IPL Image header, using a NULL tile info struct.
		BOOL rc = TRUE;
		if ( pre_allocated )
			rc = CreateImageHeaderFromIJL(&jcprops,NULL,aImage);
		else
			aImage = CreateImageHeaderFromIJL(&jcprops,NULL);
		if(NULL == aImage || rc == FALSE)
		{
			// can't create IPL image header...
			error = TRUE;
			__leave;
		}

		// allocate memory for image
		if ( ! pre_allocated )
			iplAllocateImage(aImage,0,0);
		if(NULL == aImage->imageData)
		{
			// can't allocate memory for IPL image...
			error = TRUE;
			__leave;
		}

		// tune JPEG decomressor
		jcprops.DIBBytes  = (BYTE*)aImage->imageData;
		jcprops.DIBWidth  = jcprops.JPGWidth;
		jcprops.DIBHeight  = jcprops.JPGHeight;
		jcprops.DIBChannels = 3;
		jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES(jcprops.DIBWidth,
		jcprops.DIBChannels);
		switch(jcprops.JPGChannels)
		{
		case 1:	{
					jcprops.JPGColor = IJL_G;
					break;
				}
		case 3: {
					jcprops.JPGColor = IJL_YCBCR;
					break;
				}
		default:{
					jcprops.DIBColor = (IJL_COLOR)IJL_OTHER;
					jcprops.JPGColor = (IJL_COLOR)IJL_OTHER;
					break;
				}
		}

		// read data from the JPEG into the Image
		jerr = ijlRead(&jcprops,IJL_JFILE_READWHOLEIMAGE);
		if(IJL_OK != jerr)
		{
			// can't read JPEG image data...
			error = TRUE;
			__leave;
		}

		out = aImage;

	} // try
	__finally
	{
		// release the JPEG core properties
		jerr = ijlFree(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't free IJLib...
			error = TRUE;
		}
		if(FALSE != error)
		{
			if(NULL != aImage && !pre_allocated)
			{
				iplDeallocate(aImage,IPL_IMAGE_ALL);
				aImage = NULL;
			}
		}
	}

	// TO DO: handle the condition below in a better way...
	if ( abort && out == 0 )
	{
		AfxMessageBox( "Error opening JPEG file.\n\nCheck that the file exists and that the filename is correct." );
		AfxMessageBox( filename );
		exit( 9 );
	}

	return out;
}




IplImage* ReadJpegROI( LPCSTR filename, CvRect roi, bool abort, IplImage* aImage )
{
	BOOL error = FALSE;
	BOOL pre_allocated = (aImage != 0);
	IJLERR  jerr = IJL_OK;
	IplImage* out = 0;


	// allocate the JPEG core properties
	static JPEG_CORE_PROPERTIES jcprops;
	__try
	{
		jerr = ijlInit(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't initialize IJLib...
			error = TRUE;
			__leave;
		}

		jcprops.JPGFile = const_cast<LPSTR>(filename);

		// IJL supports two different DCTs:
		// the default is fast and accurate.
		// the second (IJL_AAN) is faster but less accurate.
		// Enabling the second is done by the following line:
		// jcprops.jprops.dcttype = IJL_AAN;

		// do we have a valid ROI?
		if ( roi.width != 0 && roi.height != 0 )
		{
			IJL_RECT r;
			r.left = roi.x;
			r.top = roi.y;
			r.right = roi.x + roi.width -1;
			r.bottom = roi.y + roi.height -1;
			jcprops.jprops.roi = r;
		}

		// read image parameters: width, height,...
		jerr = ijlRead(&jcprops,IJL_JFILE_READPARAMS);
		if(IJL_OK != jerr)
		{
			// can't read JPEG image parameters...
			error = TRUE;
			__leave;
		}

		// create the IPL Image header, using a NULL tile info struct.
		jcprops.JPGWidth  = roi.width;
		jcprops.JPGHeight  = roi.height;
		jcprops.DIBWidth  = roi.width;
		jcprops.DIBHeight  = roi.height;

		BOOL rc = TRUE;
		if ( pre_allocated )
			rc = CreateImageHeaderFromIJL(&jcprops,NULL,aImage);
		else
			aImage = CreateImageHeaderFromIJL(&jcprops,NULL);

		if(NULL == aImage || rc == FALSE)
		{
			// can't create IPL image header...
			error = TRUE;
			__leave;
		}

		// allocate memory for image
		if ( !pre_allocated )
			iplAllocateImage(aImage,0,0);
		if(NULL == aImage->imageData)
		{
			// can't allocate memory for IPL image...
			error = TRUE;
			__leave;
		}

		// tune JPEG decomressor
		jcprops.DIBBytes  = (BYTE*)aImage->imageData;
		jcprops.DIBWidth  = roi.width;
		jcprops.DIBHeight  = roi.height;
		jcprops.DIBChannels = 3;
		jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES( jcprops.DIBWidth, jcprops.DIBChannels);
		switch(jcprops.JPGChannels)
		{
		case 1:	{
					jcprops.JPGColor = IJL_G;
					break;
				}
		case 3: {
					jcprops.JPGColor = IJL_YCBCR;
					break;
				}
		default:{
					jcprops.DIBColor = (IJL_COLOR)IJL_OTHER;
					jcprops.JPGColor = (IJL_COLOR)IJL_OTHER;
					break;
				}
		}

		// read data from the JPEG into the Image
		jerr = ijlRead(&jcprops,IJL_JFILE_READWHOLEIMAGE);
		if(IJL_ROI_OK != jerr)
		{
			// can't read JPEG image data...
			error = TRUE;
			__leave;
		}

		out = aImage;

	} // try
	__finally
	{
		// release the JPEG core properties
		jerr = ijlFree(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't free IJLib...
			error = TRUE;
		}
		if(FALSE != error)
		{
			if(NULL != aImage && !pre_allocated)
			{
				iplDeallocate(aImage,IPL_IMAGE_ALL);
				aImage = NULL;
			}
		}
	}

	// TO DO: handle the condition below in a better way...
	if ( abort && out == 0 )
	{
		AfxMessageBox( "Error opening JPEG file.\n\nCheck that the file exists and that the filename is correct." );
		AfxMessageBox( filename );
		exit( 9 );
	}

	return out;
}




IplImage* ReadHeaderJpeg( LPCSTR filename )
{
	BOOL error = FALSE;
	IJLERR  jerr;
	IplImage* aImage = NULL;

	// allocate the JPEG core properties
	JPEG_CORE_PROPERTIES jcprops;
	__try
	{
		jerr = ijlInit(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't initialize IJLib...
			error = TRUE;
			__leave;
		}

		jcprops.JPGFile = const_cast<LPSTR>(filename);

		// read image parameters: width, height,...
		jerr = ijlRead(&jcprops,IJL_JFILE_READPARAMS);
		if(IJL_OK != jerr)
		{
			// can't read JPEG image parameters...
			error = TRUE;
			__leave;
		}

		// create the IPL Image header, using a NULL tile info struct.
		aImage = CreateImageHeaderFromIJL( &jcprops, NULL );
		if(NULL == aImage)
		{
			// can't create IPL image header...
			error = TRUE;
			__leave;
		}

	} // try
	__finally
	{
		// release the JPEG core properties
		jerr = ijlFree(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't free IJLib...
			error = TRUE;
		}
		if(FALSE != error)
		{
			if(NULL != aImage)
			{
				iplDeallocate(aImage,IPL_IMAGE_ALL);
				aImage = NULL;
			}
		}
	}

	return aImage;
}




IplImage* ReadHeaderJpegROI( LPCSTR filename, CvRect roi )
{
	BOOL error = FALSE;
	IJLERR  jerr;
	IplImage* aImage = NULL;

	// allocate the JPEG core properties
	JPEG_CORE_PROPERTIES jcprops;
	__try
	{
		jerr = ijlInit(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't initialize IJLib...
			error = TRUE;
			__leave;
		}

		jcprops.JPGFile = const_cast<LPSTR>(filename);

		// do we have a valid ROI?
		if ( roi.width != 0 && roi.height != 0 )
		{
			IJL_RECT r;
			r.left = roi.x;
			r.top = roi.y;
			r.right = roi.x + roi.width -1;
			r.bottom = roi.y + roi.height -1;
			jcprops.jprops.roi = r;
		}

		// read image parameters: width, height,...
		jerr = ijlRead(&jcprops,IJL_JFILE_READPARAMS);
		if(IJL_OK != jerr)
		{
			// can't read JPEG image parameters...
			error = TRUE;
			__leave;
		}

		// create the IPL Image header, using a NULL tile info struct.
		jcprops.JPGWidth  = roi.width;
		jcprops.JPGHeight  = roi.height;
		jcprops.DIBWidth  = roi.width;
		jcprops.DIBHeight  = roi.height;
		aImage = CreateImageHeaderFromIJL( &jcprops, NULL );
		if(NULL == aImage)
		{
			// can't create IPL image header...
			error = TRUE;
			__leave;
		}

	} // try
	__finally
	{
		// release the JPEG core properties
		jerr = ijlFree(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't free IJLib...
			error = TRUE;
		}
		if(FALSE != error)
		{
			if(NULL != aImage)
			{
				iplDeallocate(aImage,IPL_IMAGE_ALL);
				aImage = NULL;
			}
		}
	}

	return aImage;
}




bool WriteJpeg(	IplImage* aImage,
				LPCSTR  filename )
{
	bool  bres;
	IJLERR  jerr;
	JPEG_CORE_PROPERTIES jcprops;
	bres = true;

	__try
	{
		jerr = ijlInit(&jcprops);
		if(IJL_OK != jerr)
		{
			// can't initialize IJLib...
			bres = false;
			__leave;
		}

		bres = SetJPEGProperties(jcprops,aImage);
		if(false == bres)
		{
			__leave;
		}

		jcprops.JPGFile = const_cast<LPSTR>(filename);
		jerr = ijlWrite(&jcprops,IJL_JFILE_WRITEWHOLEIMAGE);
		if(IJL_OK != jerr)
		{
			// can't write JPEG image...
			bres = false;
			__leave;
		}
	} // __try
	__finally
	{
		ijlFree(&jcprops);
	}

	return bres;
}




///////////////////////////////////////////////////////////////////////
// functions for internal use
//

IplImage* CreateImageHeaderFromIJL( const JPEG_CORE_PROPERTIES* jcprops,
									IplTileInfo* tileInfo)
{
	int channels;
	int alphach;
	char colorModel[4];
	char channelSeq[4];
	IplImage* image;

	switch(jcprops->JPGChannels)
	{
		case 1:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		case 3:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		default:
		{
			// number of channels not supported in this samples
			return NULL;
		}
	} // switch

	image = iplCreateImageHeader(
					channels,
					alphach,
					IPL_DEPTH_8U,
					colorModel,
					channelSeq,
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_TL,
					IPL_ALIGN_DWORD,
					jcprops->JPGWidth,
					jcprops->JPGHeight,
					NULL,
					NULL,
					NULL,
					tileInfo);
	return image;
}




BOOL CreateImageHeaderFromIJL( const JPEG_CORE_PROPERTIES* jcprops,
							   IplTileInfo* tileInfo,
							   IplImage* image )
{
	int channels;
	int alphach;
	char colorModel[4];
	char channelSeq[4];

	switch(jcprops->JPGChannels)
	{
		case 1:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		case 3:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		default:
		{
			// number of channels not supported in this samples
			return FALSE;
		}
	} // switch

	image = iplCreateImageHeader(
					channels,
					alphach,
					IPL_DEPTH_8U,
					colorModel,
					channelSeq,
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_TL,
					IPL_ALIGN_DWORD,
					jcprops->JPGWidth,
					jcprops->JPGHeight,
					NULL,
					NULL,
					NULL,
					tileInfo);

	return TRUE;
}




IplImage* CreateImageHeaderFromIJL( const JPEG_CORE_PROPERTIES* jcprops,
									IplTileInfo* tileInfo,
									CRect roi )
{
	int channels;
	int alphach;
	char colorModel[4];
	char channelSeq[4];
	IplImage* image;

	switch(jcprops->JPGChannels)
	{
		case 1:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		case 3:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		default:
		{
			// number of channels not supported in this samples
			return NULL;
		}
	} // switch

	image = iplCreateImageHeader(
					channels,
					alphach,
					IPL_DEPTH_8U,
					colorModel,
					channelSeq,
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_TL,
					IPL_ALIGN_DWORD,
					roi.Width(),
					roi.Height(),
					NULL,
					NULL,
					NULL,
					tileInfo);
	return image;
}




BOOL CreateImageHeaderFromIJL(	const JPEG_CORE_PROPERTIES* jcprops,
								IplTileInfo* tileInfo,
								CRect roi,
								IplImage* image )
{
	int channels;
	int alphach;
	char colorModel[4];
	char channelSeq[4];

	switch(jcprops->JPGChannels)
	{
		case 1:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		case 3:	
		{
			channels = 3;
			alphach = 0;
			colorModel[0] = channelSeq[2] = 'R';
			colorModel[1] = channelSeq[1] = 'G';
			colorModel[2] = channelSeq[0] = 'B';
			colorModel[3] = channelSeq[3] = '\0';
			break;
		}
		
		default:
		{
			// number of channels not supported in this samples
			return FALSE;
		}
	} // switch

	image = iplCreateImageHeader(
					channels,
					alphach,
					IPL_DEPTH_8U,
					colorModel,
					channelSeq,
					IPL_DATA_ORDER_PIXEL,
					IPL_ORIGIN_TL,
					IPL_ALIGN_DWORD,
					roi.Width(),
					roi.Height(),
					NULL,
					NULL,
					NULL,
					tileInfo);

	return TRUE;
}




///////////////////////////////////////////////////////////////////////
// Set JPEG properties from IplImage
//
#define IS_RGB(image) \
( image->colorModel[0] == 'R' && \
image->colorModel[1] == 'G' && \
image->colorModel[2] == 'B' )

#define IS_GRAY(image) \
( image->colorModel[0] == 'G' && \
image->colorModel[1] == 'R' && \
image->colorModel[2] == 'A' && \
image->colorModel[3] == 'Y' )

#define IS_SEQUENCE_RGB(image) \
( image->channelSeq[0] == 'R' && \
image->channelSeq[1] == 'G' && \
image->channelSeq[2] == 'B' )

#define IS_SEQUENCE_BGR(image) \
( image->channelSeq[0] == 'B' && \
image->channelSeq[1] == 'G' && \
image->channelSeq[2] == 'R' )



bool SetJPEGProperties( JPEG_CORE_PROPERTIES& jcprops,
						const IplImage*  image )
{
	if(IPL_DEPTH_8U != image->depth)
	{
		// only IPL_DEPTH_8U is supported now
		return false;
	}

	if(IPL_DATA_ORDER_PIXEL != image->dataOrder)
	{
		// only IPL_DATA_ORDER_PIXEL is supported now
		return false;
	}

	if(!IS_RGB(image) && !IS_GRAY(image))
	{
		// only RGB or GRAY color model supported in this sample
		return false;
	}

	if(image->nChannels != 1 && image->nChannels != 3)
	{
		// only 1 or 3 channels supported in this example
		return false;
	}

	jcprops.DIBChannels = image->nChannels;
	// set the color space
	// assume that 1 channel image is GRAY, and
	// 3 channel image is RGB or BGR
	switch(jcprops.DIBChannels)
	{
	case 1:
		jcprops.DIBColor = IJL_G;
		jcprops.JPGColor = IJL_G;
		jcprops.JPGChannels = 1;
		break;

	case 3:
		if(IS_SEQUENCE_RGB(image))
		{
			jcprops.DIBColor = IJL_RGB;
			jcprops.JPGColor = IJL_YCBCR;
			jcprops.JPGChannels = 3;
		}
		else if(IS_SEQUENCE_BGR(image))
		{
			jcprops.DIBColor = IJL_BGR;
			jcprops.JPGColor = IJL_YCBCR;
			jcprops.JPGChannels = 3;
		}
		else
		{
			jcprops.DIBColor = (IJL_COLOR)IJL_OTHER;
			jcprops.JPGColor = (IJL_COLOR)IJL_OTHER;
			jcprops.JPGChannels = 3;
		}
		break;

	default:
		// error for now
		break;
	}

	jcprops.DIBHeight = image->height;
	jcprops.DIBWidth = image->width;
	jcprops.JPGHeight = image->height;
	jcprops.JPGWidth = image->width;
	jcprops.JPGSubsampling = (IJL_JPGSUBSAMPLING)IJL_NONE;

	if(IPL_ORIGIN_BL == image->origin)
	{
		jcprops.DIBHeight = -jcprops.DIBHeight;
	}

	switch(image->align)
	{
	case IPL_ALIGN_4BYTES:
		jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES(jcprops.DIBWidth,
		jcprops.DIBChannels);
		break;
	case IPL_ALIGN_8BYTES:
		jcprops.DIBPadBytes = (jcprops.DIBWidth*jcprops.DIBChannels + 7)/8*8 -
		jcprops.DIBWidth*jcprops.DIBChannels;
		break;
	case IPL_ALIGN_16BYTES:
		jcprops.DIBPadBytes = (jcprops.DIBWidth*jcprops.DIBChannels + 15)/16*16 -
		jcprops.DIBWidth*jcprops.DIBChannels;
		break;
	case IPL_ALIGN_32BYTES:
		jcprops.DIBPadBytes = (jcprops.DIBWidth*jcprops.DIBChannels + 31)/32*32 -
		jcprops.DIBWidth*jcprops.DIBChannels;
		break;
	default:
		// error if go there
		break;
	}

	// set the source for the input data
	// note-this will fail if the image is tile based.
	jcprops.DIBBytes = (BYTE*)image->imageData;
	return true;
}


