
#ifndef SYNC_NODE_WITH_STATISTICS_H
#define SYNC_NODE_WITH_STATISTICS_H


#include "..\framework\SyncNode.h"
#include "CfgParameters.h"




template< class T_SOURCE_DATA, class T_SINK_DATA, class T_STATS_GATHERER >  
class SyncNodeWithStatistics : public SyncNode< T_SOURCE_DATA, T_SINK_DATA >
{
private:

	long stats_count;
	int stats_rate;

	T_STATS_GATHERER* gatherer;


public:

	SyncNodeWithStatistics( string my_name, long sync_period, 
							T_STATS_GATHERER* gatherer, long stats_period )
		:	SyncNode< T_SOURCE_DATA, T_SINK_DATA >( my_name, sync_period )
	{
		assert( gatherer );
		this->gatherer = gatherer;

		assert( stats_period > 0 );
		stats_rate = stats_period;
		stats_count = 1;
	}



	void processData()
	{
		if ( ! --stats_count )
		{
			doStatistics();
			stats_count = stats_rate;
		}

		SyncNode< T_SOURCE_DATA, T_SINK_DATA >::processData();
	}



	void doStatistics()
	{
		IplData* buffer = & source_buffers[ 0 ];
		gatherer->doStatistics( buffer );
	}

};



#endif
