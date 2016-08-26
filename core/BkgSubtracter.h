
#ifndef BKG_SUBTRACTER_H
#define BKG_SUBTRACTER_H


#pragma warning( disable : 4786 )

#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"
#include "..\tools\hsv.h"



const float MIN_SIGMA_VALUE_H = 7.0;			// degrees [0..360]
const float MIN_SIGMA_VALUE_S = 5.0;			// saturation [0..255]
const float MIN_SIGMA_VALUE_V = 5.0;			// brightness [0..255]
const float NUM_SIGMAS = 3.0;



#define USE_MASK


// these are only used for debug purposes...
//#define DUMP_ACCUMULATED_BACKGROUND
//#define DUMP_INPUT
//#define DUMP_OUTPUT




template< class ALLOCATOR >
class BkgSubtracter
	: public ProcessorNode< IplData, IplData >
{
private:

	ALLOCATOR* allocator;
	int background_frames;
	int count;

	IplImage* src_hsv;
	IplImage* bkg_rgb_mean;
	IplImage* bkg_hsv_mean;		// background mean
	IplImage* bkg_hsv_dev3;		// background 3-sigma standard deviation
	IplImage* bkg_res;



	CvFont my_font;


	double foregrnd_update;
	double backgrnd_update;

	int i_foregrnd_update;
	int i_backgrnd_update;
	int i_inv_foregrnd_update;		// = ( 1 - i_foregrnd_update )
	int i_inv_backgrnd_update;		// = ( 1 - i_backgrnd_update )

	bool do_update_backgrnd;
	bool do_update_foregrnd_pixel;
	bool do_update_backgrnd_pixel;

	enum {	UPDATE_RATE = 6 };
	long last_updated;


	double max_shadow_darkening;
	bool detect_shadows;


	long max_motion_pixels;
	bool bkg_model_failure;


public:

	enum {	FLOAT_RES = 1000 };

	enum {	HALF_CIRCLE = 180 * FLOAT_RES,
			FULL_CIRCLE = 360 * FLOAT_RES };

	enum {	SHADOW_PIXEL		= 150,
			FOREGROUND_PIXEL	= 255,
			PROBABLE_PIXEL		= 35,
			BACKGROUND_PIXEL	= 0 };


	static HANDLE ready_signal;




	BkgSubtracter( string my_name, int background_frames, ALLOCATOR* allocator )
		:	ProcessorNode< IplData, IplData >( my_name )
	{
		assert( allocator );
		this->allocator = allocator;

		this->background_frames = background_frames;
		assert( background_frames > 0 );

		count = 0;


		src_hsv = 0;
		bkg_hsv_mean = 0;
		bkg_rgb_mean = 0;
		bkg_hsv_dev3 = 0;
		bkg_res = 0;


		if ( ready_signal == 0 )
		{
			ready_signal = CreateEvent( NULL, 
										false,		// automatic reset
										false,		// initially NOT signalled
										NULL );
		}
		assert( ready_signal );
		SetEvent( ready_signal );


		cvInitFont( &my_font, CV_FONT_VECTOR0, 0.5, 0.5, 0, 1 );


		// get algorithm parameters
		max_shadow_darkening = CfgParameters::instance()->getMaxShadowDarkening();
		detect_shadows = max_shadow_darkening < 1.0;


		// background update
		foregrnd_update = CfgParameters::instance()->getForegroundPxlUpdate();
		backgrnd_update = CfgParameters::instance()->getBackgroundPxlUpdate();

		double ef = 0.0, eb = 0.0;
		for ( int i = 0; i < UPDATE_RATE; ++i )
		{
			ef += foregrnd_update * pow( ( 1.0 - foregrnd_update ), i );
			eb += backgrnd_update * pow( ( 1.0 - backgrnd_update ), i );
		}

		foregrnd_update = ef;
		backgrnd_update = eb;
		last_updated = 1;


		// pre-converting to int to avoid using floating point 
		// operations during background subtraction.
		i_foregrnd_update = lrintf( foregrnd_update * FLOAT_RES );
		i_backgrnd_update = lrintf( backgrnd_update * FLOAT_RES );

		i_inv_foregrnd_update = FLOAT_RES - i_foregrnd_update;
		i_inv_backgrnd_update = FLOAT_RES - i_backgrnd_update;	

		do_update_foregrnd_pixel = i_foregrnd_update > 0;
		do_update_backgrnd_pixel = i_backgrnd_update > 0;
		do_update_backgrnd = do_update_foregrnd_pixel || do_update_backgrnd_pixel;


		max_motion_pixels = 0;
		bkg_model_failure = false;
	}



	~BkgSubtracter()
	{
		if ( bkg_rgb_mean != 0 )
			allocator->release( bkg_rgb_mean );
		if ( bkg_hsv_mean != 0 )
			allocator->release( bkg_hsv_mean );
		if ( bkg_hsv_dev3 != 0 )
			allocator->release( bkg_hsv_dev3 );
		if ( bkg_res != 0 )
			allocator->release( bkg_res );
		if ( src_hsv != 0 )
			allocator->release( src_hsv );
	}
	


	virtual void prepareForInput()
	{
		++count;
		ProcessorNode< IplData, IplData >::prepareForInput();
	}



	virtual void getCallback( IplData* d, int token )
	{
		ProcessorNode< IplData, IplData >::getCallback( d, token );
		allocator->use( d->img );
	}



	void processData()
	{
		IplImage* src = source_buffers[ 0 ].img;
		assert( src );
		assert( src->nChannels == 3 );


#ifdef USE_MASK
		IplImage* mask = CfgParameters::instance()->getMask();
#else
		IplImage* mask = 0;
#endif


		if ( count == 1 )				// create buffers
		{
			assert( bkg_rgb_mean == 0 );
			assert( bkg_hsv_mean == 0 );
			assert( bkg_hsv_dev3 == 0 );
			assert( bkg_res == 0 );

			IplImageResource typ = IplImageResource::getType( src );
			typ.channels = 3;
			typ.depth = IPL_DEPTH_32S;

			bkg_rgb_mean = allocator->acquireClean( typ );
			assert( bkg_rgb_mean );

			bkg_hsv_mean = allocator->acquireClean( typ );
			assert( bkg_hsv_mean );

			bkg_hsv_dev3 = allocator->acquireClean( typ );
			assert( bkg_hsv_dev3 );


			typ.channels = 1;
			typ.depth = IPL_DEPTH_8U;

			bkg_res = allocator->acquireClean( typ );
			assert( bkg_res );


			// calculate the allowed maximum number of motion pixels 
			// from the total number of potential motion pixels (i.e. being checked)
			if ( mask != 0 )
			{
				max_motion_pixels = cvCountNonZero( mask );
			}
			else
			{
				max_motion_pixels = src->width * src->height;
			}
			max_motion_pixels /= 2;				// 0.5 threshold i.e. half the picture
		}



		// colour conversion
		IplImageResource typ = IplImageResource::getType( src );
		typ.depth = IPL_DEPTH_32S;

		src_hsv = allocator->acquireClean( typ );
		assert( src_hsv );

		convertImageRGB2HSV( src, src_hsv, mask );


		
		// has sink finished using/updating our internal background model buffers?
		assert( ready_signal );
		WaitForSingleObject( ready_signal, INFINITE );
		ResetEvent( ready_signal );

			

		//--------------- INITIAL BACKGROUND ACCUMULATION -----------------------------
		if ( count <= background_frames )
		{

			int y = src->height;
			do
			{
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_dev3, bkg_hsv_dev3, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_rgb_mean, bkg_rgb_mean, y-1 );
#ifdef USE_MASK
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );
#endif

				int x = src->width;
				do
				{
#ifdef USE_MASK
					if ( p_msk[ x-1 ] )
#endif
					{
						int x3 = (x-1) * 3;

						unsigned char b = p_src[ x3 ];
						unsigned char g = p_src[ x3 +1 ];
						unsigned char r = p_src[ x3 +2 ];

						int h = p_src_hsv[ x3    ];
						int s = p_src_hsv[ x3 +1 ];
						int v = p_src_hsv[ x3 +2 ];

						// sum
						p_bkg_hsv_mean[ x3    ] += h;	// max value = 360 * BKGFRAMES
						p_bkg_hsv_mean[ x3 +1 ] += s;	// max value = 255 * BKGFRAMES
						p_bkg_hsv_mean[ x3 +2 ] += v;	// max value = 255 * BKGFRAMES

						p_bkg_rgb_mean[ x3    ] += b;	// max value = 255 * BKGFRAMES
						p_bkg_rgb_mean[ x3 +1 ] += g;	// max value = 255 * BKGFRAMES
						p_bkg_rgb_mean[ x3 +2 ] += r;	// max value = 255 * BKGFRAMES

						// sum of squares
						p_bkg_hsv_dev3[ x3    ] += h * h;	// max value = 360 * 360 * BKGFRAMES
						p_bkg_hsv_dev3[ x3 +1 ] += s * s;	// max value = 255 * 255 * BKGFRAMES
						p_bkg_hsv_dev3[ x3 +2 ] += v * v;	// max value = 255 * 255 * BKGFRAMES
					}
				}
				while( --x );
			}
			while( --y );


			// keep user informed...
			IplImageResource typ = IplImageResource::getType( src );

			char tmp[ 100 ];
			sprintf( tmp, "Accumulating background image.  %d...", 
					 background_frames - count );
			iplSet( bkg_res, 0 );
			cvPutText(	bkg_res, tmp, cvPoint( 10, 50 ),
						&my_font, CV_RGB(255,255,255) );		//MM! 0xffffff );

		}
		else if ( count == background_frames +1 )
		{
			//------- FINALISING THE INITIAL BACKGROUND -----------------------------
			int y = src->height;
			do
			{
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_dev3, bkg_hsv_dev3, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_rgb_mean, bkg_rgb_mean, y-1 );
#ifdef USE_MASK
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );
#endif

				int x = src->width;
				do
				{
					int x3 = (x-1) * 3;
#ifdef USE_MASK
					if ( p_msk[ x-1 ] )
					{
#endif
						// mean
						float mh = p_bkg_hsv_mean[ x3    ] / (float) background_frames;
						float ms = p_bkg_hsv_mean[ x3 +1 ] / (float) background_frames;
						float mv = p_bkg_hsv_mean[ x3 +2 ] / (float) background_frames;

						float mvb = p_bkg_rgb_mean[ x3    ] / (float) background_frames;
						float mvg = p_bkg_rgb_mean[ x3 +1 ] / (float) background_frames;
						float mvr = p_bkg_rgb_mean[ x3 +2 ] / (float) background_frames;

						// standard deviation
						float sdh = sqrtf( p_bkg_hsv_dev3[ x3    ] / (float) background_frames - mh * mh );
						float sds = sqrtf( p_bkg_hsv_dev3[ x3 +1 ] / (float) background_frames - ms * ms );
						float sdv = sqrtf( p_bkg_hsv_dev3[ x3 +2 ] / (float) background_frames - mv * mv );

						// make sure std dev sigma threshold values are not very low (allowance for camera noise)
						sdh = sdh < MIN_SIGMA_VALUE_H ? MIN_SIGMA_VALUE_H : sdh;
						sds = sds < MIN_SIGMA_VALUE_S ? MIN_SIGMA_VALUE_S : sds;
						sdv = sdv < MIN_SIGMA_VALUE_V ? MIN_SIGMA_VALUE_V : sdv;

						// values in bkg and std_dev images, have the format:
						// ...nnnnmmm where value = ...nnnn.mmm
						p_bkg_hsv_mean[ x3    ] = lrintf( mh * FLOAT_RES );
						p_bkg_hsv_mean[ x3 +1 ] = lrintf( ms * FLOAT_RES );
						p_bkg_hsv_mean[ x3 +2 ] = lrintf( mv * FLOAT_RES );

						p_bkg_rgb_mean[ x3    ] = lrintf( mvb * FLOAT_RES );
						p_bkg_rgb_mean[ x3 +1 ] = lrintf( mvg * FLOAT_RES );
						p_bkg_rgb_mean[ x3 +2 ] = lrintf( mvr * FLOAT_RES );

						p_bkg_hsv_dev3[ x3    ] = lrintf( NUM_SIGMAS * sdh * FLOAT_RES );
						p_bkg_hsv_dev3[ x3 +1 ] = lrintf( NUM_SIGMAS * sds * FLOAT_RES );
						p_bkg_hsv_dev3[ x3 +2 ] = lrintf( NUM_SIGMAS * sdv * FLOAT_RES );

#ifdef USE_MASK
					}
					else
					{
						p_bkg_hsv_mean[ x3    ] = 0;
						p_bkg_hsv_mean[ x3 +1 ] = 0;
						p_bkg_hsv_mean[ x3 +2 ] = 0;

						p_bkg_hsv_dev3[ x3    ] = 0;
						p_bkg_hsv_dev3[ x3 +1 ] = 0;
						p_bkg_hsv_dev3[ x3 +2 ] = 0;

						p_bkg_rgb_mean[ x3    ] = 0;
						p_bkg_rgb_mean[ x3 +1 ] = 0;
						p_bkg_rgb_mean[ x3 +2 ] = 0;
					}
#endif
				}
				while( --x );
			}
			while( --y );


#ifdef DUMP_ACCUMULATED_BACKGROUND
			{
				IplImage* tmp = cvCreateImage( cvSize( src->width, src->height ), IPL_DEPTH_8U, 3 );
				cvConvertScale( bkg_rgb_mean, tmp, 1.0 / FLOAT_RES );
				cvSaveImage( "\\out\\bkg.bmp", tmp );
				cvConvertScale( bkg_hsv_mean, tmp, 1.0 / ( 2 * FLOAT_RES ) );  // to fit H [0..360] in [0..180]
				cvSaveImage( "\\out\\bkg_hsv_halfed.bmp", tmp );
				cvConvertScale( bkg_hsv_mean, tmp, 1.0 / FLOAT_RES );	// for rest: S, V
				cvSaveImage( "\\out\\bkg_hsv.bmp", tmp );
				cvConvertScale( bkg_hsv_dev3, tmp, 1.0 / FLOAT_RES );
				cvSaveImage( "\\out\\bkg_hsv_dev3.bmp", tmp );
				cvConvertScale( bkg_hsv_dev3, tmp, 1.0 / ( 2 * FLOAT_RES ) );
				cvSaveImage( "\\out\\bkg_hsv_dev3_halfed.bmp", tmp );
				cvReleaseImage( &tmp );
			}
#endif

		}
		else
		{
			//------------------- BACKGROUND SUBTRACTION ------------------------------

			const int LOW_HSV_THRESHOLD	= 0.02 * 256 * FLOAT_RES;
			const int HIGH_HSV_THRESHOLD = 256 * FLOAT_RES - LOW_HSV_THRESHOLD;


			long motion_pixels = 0;


			int y = bkg_res->height;
			do
			{
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_dev3, bkg_hsv_dev3, y-1 );
				COMPUTE_IMAGE_ROW_PTR( int, p_bkg_rgb_mean, bkg_rgb_mean, y-1 );
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_bkg_res, bkg_res, y-1 );
#ifdef USE_MASK
				COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );
#endif

				int x = bkg_res->width;
				do
				{
#ifdef USE_MASK
					if ( p_msk[ x-1 ] )
					{
#endif
						int x3 = ( x - 1 ) * 3;


						int b = p_src[ x3    ] * FLOAT_RES;
						int g = p_src[ x3 +1 ] * FLOAT_RES;
						int r = p_src[ x3 +2 ] * FLOAT_RES;

						int diff_vb = abs( b - p_bkg_rgb_mean[ x3   ] );
						int diff_vg = abs( g - p_bkg_rgb_mean[ x3+1 ] );
						int diff_vr = abs( r - p_bkg_rgb_mean[ x3+2 ] );
						int diff_v = MAX( MAX( diff_vb, diff_vg ), diff_vr );


						int h = p_src_hsv[ x3    ];
						int s = p_src_hsv[ x3 +1 ];
						int v = p_src_hsv[ x3 +2 ];

						h = h * FLOAT_RES;
						s = s * FLOAT_RES;
						v = v * FLOAT_RES;

						int diff_h = abs( h - p_bkg_hsv_mean[ x3 ] );


						// h ranges from [0..360] and wraps around,
						// so must choose smallest distance
						if ( diff_h > HALF_CIRCLE )
							diff_h = FULL_CIRCLE - diff_h;


						if ( diff_v > p_bkg_hsv_dev3[ x3 +2 ] )
						{
							if ( detect_shadows &&
								 v < p_bkg_hsv_mean[ x3 +2 ] &&								// darker (lower brightness)
								 v > ( max_shadow_darkening * p_bkg_hsv_mean[ x3 +2 ] ) &&	// but not too dark
								 ( diff_h < p_bkg_hsv_dev3[ x3 ] || s < 30000 ) )			// nearly same hue	
							{
								// shadow pixel
								p_bkg_res[ x-1 ] = SHADOW_PIXEL;
							}
							else if ( diff_v > 4.0 / 3.0 * p_bkg_hsv_dev3[ x3 +2 ] )
							{
								// foreground pixel
								p_bkg_res[ x-1 ] = FOREGROUND_PIXEL;
								++motion_pixels;
							}
							else 
							{
								// probable foreground pixel
								p_bkg_res[ x-1 ] = PROBABLE_PIXEL;
							}
						}
						else if ( v > LOW_HSV_THRESHOLD && 
								  s > LOW_HSV_THRESHOLD && 
								  diff_h > p_bkg_hsv_dev3[ x3 ] )
						{
							if ( diff_h > 4 * p_bkg_hsv_dev3[ x3 ] )
							{
								// foreground pixel
								p_bkg_res[ x-1 ] = FOREGROUND_PIXEL;
								++motion_pixels;
							}
							else 
							{
								// probable foreground pixel
								p_bkg_res[ x-1 ] = PROBABLE_PIXEL;
							}
						}
						else
						{
							// background pixel
							p_bkg_res[ x-1 ] = BACKGROUND_PIXEL;
						}
#ifdef USE_MASK
					}
					else
					{
						p_bkg_res[ x-1 ] = BACKGROUND_PIXEL;
					}
#endif
				}
				while( --x );
			}
			while( --y );


			//------ thresholding with hysterisis
			PRE_COMPUTE_IMAGE_ROW_PTRS( unsigned char, p_bkg_res, bkg_res );

			const int NEIGHBOURS_THRESHOLD = 4;
			unsigned char iterations = 4;
			bool changes = false;
			bool toggle = false;

			do
			{
				changes = false;

				for ( y = 1; y < src->height -1; y++ )
				{
#ifdef USE_MASK
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y );
#endif
					if ( toggle )
					{
						for ( int x = src->width -2; x > 0; --x )
						{
#ifdef USE_MASK
							if ( p_msk[ x ] )
#endif
							{
								unsigned char p0 = p_bkg_res[ y ][ x ];

								if ( p0 != BACKGROUND_PIXEL )
								{				
									unsigned char foreground_pxls = 0;
									unsigned char shadow_pxls = 0;


#define CHECK_PXL( PXL )					\
			p = PXL;						\
			if ( p == FOREGROUND_PIXEL )	\
				++foreground_pxls;			\
			else if ( p == SHADOW_PIXEL )	\
				++shadow_pxls;

									unsigned char p;
									CHECK_PXL( p_bkg_res[ y - 1 ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y - 1 ][ x     ] );
									CHECK_PXL( p_bkg_res[ y - 1 ][ x + 1 ] );
									CHECK_PXL( p_bkg_res[ y     ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y     ][ x + 1 ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x     ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x + 1 ] );


									// demote isolated pixel(s)
									if ( p0 == FOREGROUND_PIXEL && 
										 foreground_pxls < NEIGHBOURS_THRESHOLD )
									{
										p0 = p_bkg_res[ y ][ x ] = PROBABLE_PIXEL;
										changes = true;
										--motion_pixels;
									}
									else if ( p0 == SHADOW_PIXEL && 
											  shadow_pxls < NEIGHBOURS_THRESHOLD )
									{
										p0 = p_bkg_res[ y ][ x ] = PROBABLE_PIXEL;
										changes = true;
									}

									// promote probable foreground pixels
									if ( p0 == PROBABLE_PIXEL && 
										 foreground_pxls >= NEIGHBOURS_THRESHOLD && 
										 foreground_pxls > shadow_pxls )
									{
										p_bkg_res[ y ][ x ] = FOREGROUND_PIXEL;
										changes = true;
										++motion_pixels;
									}

									// promote probable shadow pixels
									if ( p0 == PROBABLE_PIXEL && 
										 shadow_pxls >= NEIGHBOURS_THRESHOLD && 
										 shadow_pxls > foreground_pxls )
									{
										p_bkg_res[ y ][ x ] = SHADOW_PIXEL;
										changes = true;
									}
								}
							}
						}
					}
					else
					{
						for ( int x = 1; x < ( src->width -1 ); ++x )
						{
#ifdef USE_MASK
							if ( p_msk[ x ] )
#endif
							{
								unsigned char p0 = p_bkg_res[ y ][ x ];

								if ( p0 != BACKGROUND_PIXEL )
								{				
									unsigned char foreground_pxls = 0;
									unsigned char shadow_pxls = 0;


									unsigned char p;
									CHECK_PXL( p_bkg_res[ y - 1 ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y - 1 ][ x     ] );
									CHECK_PXL( p_bkg_res[ y - 1 ][ x + 1 ] );
									CHECK_PXL( p_bkg_res[ y     ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y     ][ x + 1 ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x - 1 ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x     ] );
									CHECK_PXL( p_bkg_res[ y + 1 ][ x + 1 ] );
#undef CHECK_PXL

									// demote isolated pixel(s)
									if ( p0 == FOREGROUND_PIXEL && 
										 foreground_pxls < NEIGHBOURS_THRESHOLD )
									{
										p0 = p_bkg_res[ y ][ x ] = PROBABLE_PIXEL;
										changes = true;
										--motion_pixels;
									}
									else if ( p0 == SHADOW_PIXEL && 
											  shadow_pxls < NEIGHBOURS_THRESHOLD )
									{
										p0 = p_bkg_res[ y ][ x ] = PROBABLE_PIXEL;
										changes = true;
									}

									// promote probable foreground pixels
									if ( p0 == PROBABLE_PIXEL && 
										 foreground_pxls >= NEIGHBOURS_THRESHOLD && 
										 foreground_pxls > shadow_pxls )
									{
										p_bkg_res[ y ][ x ] = FOREGROUND_PIXEL;
										changes = true;
										++motion_pixels;
									}

									// promote probable shadow pixels
									if ( p0 == PROBABLE_PIXEL && 
										 shadow_pxls >= NEIGHBOURS_THRESHOLD && 
										 shadow_pxls > foreground_pxls )
									{
										p_bkg_res[ y ][ x ] = SHADOW_PIXEL;
										changes = true;
									}
								}
							}
						}
					}
				}

				toggle = ! toggle;
			}
			while( iterations-- && changes );



			//------ check if too many changes ------
			if ( motion_pixels > max_motion_pixels )
			{
				CLOG( "Background Model Failure -- Too Many Changes!", 0x0000ee );
				bkg_model_failure = true;
			}




			//------ update background -------
			if ( do_update_backgrnd && ( last_updated++ % UPDATE_RATE == 0 ) )
			{
				int y = bkg_res->height;
				do
				{
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_src, src, y-1 );
					COMPUTE_IMAGE_ROW_PTR( int, p_src_hsv, src_hsv, y-1 );
					COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_mean, bkg_hsv_mean, y-1 );
					COMPUTE_IMAGE_ROW_PTR( int, p_bkg_hsv_dev3, bkg_hsv_dev3, y-1 );
					COMPUTE_IMAGE_ROW_PTR( int, p_bkg_rgb_mean, bkg_rgb_mean, y-1 );
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_bkg_res, bkg_res, y-1 );
#ifdef USE_MASK
					COMPUTE_IMAGE_ROW_PTR( unsigned char, p_msk, mask, y-1 );
#endif

					int x = bkg_res->width;
					do
					{
#ifdef USE_MASK
						if ( p_msk[ x-1 ] )
#endif
						{
							int x3 = (x-1) * 3;


							int b = p_src[ x3    ];
							int g = p_src[ x3 +1 ];
							int r = p_src[ x3 +2 ];

							int h = p_src_hsv[ x3    ];
							int s = p_src_hsv[ x3 +1 ];
							int v = p_src_hsv[ x3 +2 ];


							h = h * FLOAT_RES;
							s = s * FLOAT_RES;
							v = v * FLOAT_RES;

							r = r * FLOAT_RES;
							g = g * FLOAT_RES;
							b = b * FLOAT_RES;

							
							static int svf = NUM_SIGMAS * MIN_SIGMA_VALUE_V * FLOAT_RES;
							static int ssf = NUM_SIGMAS * MIN_SIGMA_VALUE_S * FLOAT_RES;
							static int shf = NUM_SIGMAS * MIN_SIGMA_VALUE_H * FLOAT_RES;


							// update background
							if ( p_bkg_res[ x-1 ] == FOREGROUND_PIXEL )
							{
								if ( do_update_foregrnd_pixel )
								{
									int bkg_h = p_bkg_hsv_mean[ x3    ];
									int bkg_s = p_bkg_hsv_mean[ x3 +1 ];
									int bkg_v = p_bkg_hsv_mean[ x3 +2 ];

									bkg_h = ( i_foregrnd_update * h 
											  + i_inv_foregrnd_update * bkg_h ) / FLOAT_RES;
									bkg_s = ( i_foregrnd_update * s 
											  + i_inv_foregrnd_update * bkg_s ) / FLOAT_RES;
									bkg_v = ( i_foregrnd_update * v 
											  + i_inv_foregrnd_update * bkg_v ) / FLOAT_RES;

									p_bkg_hsv_mean[ x3    ] = bkg_h;
									p_bkg_hsv_mean[ x3 +1 ] = bkg_s;
									p_bkg_hsv_mean[ x3 +2 ] = bkg_v;



									int dev3_h = p_bkg_hsv_dev3[ x3    ];
									int dev3_s = p_bkg_hsv_dev3[ x3 +1 ];
									int dev3_v = p_bkg_hsv_dev3[ x3 +2 ];

									static int fs = i_foregrnd_update * NUM_SIGMAS;

									dev3_h = ( fs * abs( h - bkg_h )
											   + i_inv_foregrnd_update * dev3_h ) / FLOAT_RES;
									dev3_s = ( fs * abs( s - bkg_s )
											   + i_inv_foregrnd_update * dev3_s ) / FLOAT_RES;
									dev3_v = ( fs * abs( v - bkg_v ) 
											   + i_inv_foregrnd_update * dev3_v ) / FLOAT_RES;

									if ( dev3_h < shf )
										dev3_h = shf;
									if ( dev3_s < ssf )
										dev3_s = ssf;
									if ( dev3_v < svf )
										dev3_v = svf;

									p_bkg_hsv_dev3[ x3    ] = dev3_h;
									p_bkg_hsv_dev3[ x3 +1 ] = dev3_s;
									p_bkg_hsv_dev3[ x3 +2 ] = dev3_v;



									p_bkg_rgb_mean[ x3 +2 ] = ( i_foregrnd_update * r 
															+ i_inv_foregrnd_update * p_bkg_rgb_mean[ x3 +2 ] ) / FLOAT_RES;
									p_bkg_rgb_mean[ x3 +1 ] = ( i_foregrnd_update * g 
															+ i_inv_foregrnd_update * p_bkg_rgb_mean[ x3 +1 ] ) / FLOAT_RES;
									p_bkg_rgb_mean[ x3    ] = ( i_foregrnd_update * b 
															+ i_inv_foregrnd_update * p_bkg_rgb_mean[ x3    ] ) / FLOAT_RES;
								}
							}
							else 
							{
								if ( do_update_backgrnd_pixel )
								{
									int bkg_h = p_bkg_hsv_mean[ x3    ];
									int bkg_s = p_bkg_hsv_mean[ x3 +1 ];
									int bkg_v = p_bkg_hsv_mean[ x3 +2 ];

									bkg_h = ( i_backgrnd_update * h 
											  + i_inv_backgrnd_update * bkg_h ) / FLOAT_RES;
									bkg_s = ( i_backgrnd_update * s 
											  + i_inv_backgrnd_update * bkg_s ) / FLOAT_RES;
									bkg_v = ( i_backgrnd_update * v 
											  + i_inv_backgrnd_update * bkg_v ) / FLOAT_RES;

									p_bkg_hsv_mean[ x3    ] = bkg_h;
									p_bkg_hsv_mean[ x3 +1 ] = bkg_s;
									p_bkg_hsv_mean[ x3 +2 ] = bkg_v;



									int dev3_h = p_bkg_hsv_dev3[ x3    ];
									int dev3_s = p_bkg_hsv_dev3[ x3 +1 ];
									int dev3_v = p_bkg_hsv_dev3[ x3 +2 ];

									static int bs = i_backgrnd_update * NUM_SIGMAS;

									dev3_h = ( bs * abs( h - bkg_h )
											   + i_inv_backgrnd_update * dev3_h ) / FLOAT_RES;
									dev3_s = ( bs * abs( s - bkg_s )
											   + i_inv_backgrnd_update * dev3_s ) / FLOAT_RES;
									dev3_v = ( bs * abs( v - bkg_v )
											   + i_inv_backgrnd_update * dev3_v ) / FLOAT_RES;

									if ( dev3_h < shf )
										dev3_h = shf;
									if ( dev3_s < ssf )
										dev3_s = ssf;
									if ( dev3_v < svf )
										dev3_v = svf;

									p_bkg_hsv_dev3[ x3    ] = dev3_h;
									p_bkg_hsv_dev3[ x3 +1 ] = dev3_s;
									p_bkg_hsv_dev3[ x3 +2 ] = dev3_v;



									p_bkg_rgb_mean[ x3 +2 ] = ( i_backgrnd_update * r 
															+ i_inv_backgrnd_update * p_bkg_rgb_mean[ x3 +2 ] ) / FLOAT_RES;
									p_bkg_rgb_mean[ x3 +1 ] = ( i_backgrnd_update * g 
															+ i_inv_backgrnd_update * p_bkg_rgb_mean[ x3 +1 ] ) / FLOAT_RES;
									p_bkg_rgb_mean[ x3    ] = ( i_backgrnd_update * b 
															+ i_inv_backgrnd_update * p_bkg_rgb_mean[ x3    ] ) / FLOAT_RES;
								}
							}
						}
					}
					while( --x );
				}
				while( --y );

				last_updated = 1;		// reset to prevent overflow for long runs
			}

		}


#ifdef DUMP_INPUT
		if ( count > background_frames +1 )
		{
			char tmp[200];
			sprintf( tmp, "\\out\\%04.04d_src.bmp", source_buffers[ 0 ].frame );
			cvSaveImage( tmp, src );
		}
#endif

#ifdef DUMP_OUTPUT		
		if ( count > background_frames +1 )
		{
			char tmp[200];
			sprintf( tmp, "\\out\\%04.04d_bkg_res.bmp", source_buffers[ 0 ].frame );
			cvSaveImage( tmp, bkg_res );
		}
#endif



		// push result to sink buffer(s)
		int num_buffers = sink_buffers.size();
		assert( num_buffers <= 5 );

		if ( num_buffers )
		{
			IplData out1;
			out1.img = bkg_res;
			out1.ignore = count < background_frames +2;
			out1.frame = source_buffers[ 0 ].frame;

			// note that here we are passing a pointer to a local copy of our
			// buffers. therefore, we should wait until the sink node sends a 
			// signal before we re-use/change the buffers. same applies for the
			// remaining buffers sent to the rest of the sink node(s).

			sink_buffers[ 0 ] = out1;
		}
		if ( num_buffers > 1 )
		{
			IplData out1;
			out1.img = bkg_rgb_mean;
			out1.ignore = count < background_frames +2;
			out1.frame = source_buffers[ 0 ].frame;
		
			sink_buffers[ 1 ] = out1;
		}
		if ( num_buffers > 2 )
		{
			IplData out1;
			out1.img = bkg_hsv_mean;
			out1.ignore = count < background_frames +2;
			out1.frame = source_buffers[ 0 ].frame;
		
			sink_buffers[ 2 ] = out1;
		}
		if ( num_buffers > 3 )
		{
			IplData out1;
			out1.img = src;
			out1.ignore = count < background_frames +2;
			out1.frame = source_buffers[ 0 ].frame;
		
			sink_buffers[ 3 ] = out1;
		}
		if ( num_buffers > 4 )
		{
			IplData out1;
			out1.img = src_hsv;
			out1.ignore = count < background_frames +2;
			out1.frame = source_buffers[ 0 ].frame;
		
			sink_buffers[ 4 ] = out1;
		}

	}



	virtual void push()
	{
		if ( bkg_res )
			ProcessorNode< IplData, IplData >::push();
	}



	virtual void finalise()
	{
		ProcessorNode< IplData, IplData >::finalise();

		for ( int k = 0; k < source_buffers.size(); ++k )
			if ( source_buffers[k].img )		// in case source node died
				allocator->release( source_buffers[k].img );

		if ( src_hsv )
		{
			allocator->release( src_hsv );
			src_hsv = 0;
		}

		// terminate this node if motion detection fails...
		if ( bkg_model_failure )
			terminate( RC_ERROR );
	}

};



template< class ALLOCATOR >
HANDLE BkgSubtracter< ALLOCATOR >::ready_signal = 0;




#endif
