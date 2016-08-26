

#include "IplImageResource.h"




long IplImageResource::memoryUsed()
{
	return width * height * channels * ( depth & IPL_DEPTH_MASK ) / 8 + sizeof IplImage;
}



ostream& operator<< ( ostream& strm, const IplImageResource& res )
{
	strm << res.width << " x " << res.height << "  " 
		 << ( res.depth & IPL_DEPTH_MASK )
		 << " bpp  " << res.channels << " channels";

	return strm;
}



bool operator== ( const IplImageResource& r1, const IplImageResource& r2 )
{
	return	r1.width == r2.width &&
			r1.height == r2.height &&
			r1.depth == r2.depth &&
			r1.channels == r2.channels;
}


