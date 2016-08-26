
#ifndef TIMER_H
#define TIMER_H


#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#include <windows.h>



class Timer
{
private:

	LARGE_INTEGER startTime, stopTime;
	LARGE_INTEGER freq;

public:

	Timer()
	{
		QueryPerformanceFrequency( &freq );
		startTime.LowPart = startTime.HighPart = 0;
		stopTime.LowPart = stopTime.HighPart = 0;
	}

	inline void start()
	{
		QueryPerformanceCounter( &startTime );
	}


	inline void stop()
	{
		QueryPerformanceCounter( &stopTime );
	}


	inline double duration()				// in seconds
	{
		return (double) (stopTime.QuadPart - startTime.QuadPart ) / freq.QuadPart;
	}


	inline double frequency()
	{
		return (double) freq.QuadPart;
	}

};





#define TIME_PROFILING_START	static double _tt_total = 0;		\
								static long _tt_count = -1;			\
								Timer _tt;							\
								_tt.start();

#define TIME_PROFILING_END		_tt.stop();							\
								++_tt_count;						\
								if ( _tt_count > 0 )				\
									_tt_total += _tt.duration();	\
								LOG( "!time = " << _tt.duration() << " sec.   (avg = " << _tt_total/_tt_count << ")" );



#endif
