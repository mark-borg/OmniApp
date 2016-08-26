
#ifndef PROCESSOR_NODE_H
#define PROCESSOR_NODE_H


#include "RunnableObject.h"
#include <afxmt.h>					// multithreading stuff
#include <vector>
#include <strstream>

using namespace std;





template< class T_SOURCE_DATA, class T_SINK_DATA >  
class ProcessorNode
	: public RunnableObject
{
public:

	struct Link
	{
		ProcessorNode* node;
		int node_token;
		
		HANDLE received_data;		// used to signal when an object gets data from 
									// its source.
		HANDLE not_busy;			// used to send a signal to a source that it should 
									// push data, as this node is no longer busy.
		bool sink_is_alive;			// used internally by a node to flag that a sink
									// node has died (its thread is no longer running).

		Link()
		{
			node = 0; 
			node_token = -1;
			received_data = not_busy = NULL;

			sink_is_alive = true;
		}


		Link& copyFrom( const Link& other_lnk )
		{
			received_data	= other_lnk.received_data;
			not_busy		= other_lnk.not_busy;

			return *this;
		}


		Link( ProcessorNode* node )
		{
			this->node = node;

			node_token = -1;
			sink_is_alive = true;

			// common events
			received_data = CreateEvent(	NULL, 
											false,		// automatic reset
											false,		// initially NOT signalled
											NULL );
			assert( received_data );

			not_busy	  = CreateEvent(	NULL, 
											false,		// automatic reset
											false,		// initially NOT signalled
											NULL );
			assert( not_busy );
		}
	};


	typedef vector< Link >						LinkList;
	typedef typename LinkList::iterator			LinkIterator;

	typedef vector< T_SOURCE_DATA >				SourceDataList;
	typedef typename SourceDataList::iterator	SourceDataIterator;

	typedef vector< T_SINK_DATA >				SinkDataList;
	typedef typename SinkDataList::iterator		SinkDataIterator;

	LinkList	sources;
	LinkList	sinks;

	SourceDataList	source_buffers;
	SinkDataList	sink_buffers;

	string my_name;

	CCriticalSection safe_state;			// signalled when safe to suspend
	HANDLE dying;							// signalled when object is shutting down


	float orig_process_rate;
	int actual_process_rate;
	int count;

	bool is_hidden;			// data can be ignored, as this node's results are hidden
	bool high_priority;		// this node cannot lower its process rate



	ProcessorNode( string name )
	{
		my_name = name;

		// common events
		dying = CreateEvent(	NULL, 
								false,		// automatic reset
								false,		// initially NOT signalled
								NULL );
		assert( dying );

		orig_process_rate = 1.0;
		actual_process_rate = 1;
		count = actual_process_rate - 1;
		is_hidden = false;
		high_priority = true;
	}



	bool isHidden()
	{
		return is_hidden;
	}



	void Hide()
	{
		is_hidden = true;
	}



	void Show()
	{
		is_hidden = false;
	}



	void changeRate( float new_rate )
	{
		assert( new_rate > 0.0 );
		orig_process_rate = new_rate;
		actual_process_rate = ( new_rate < 1.0 ? 1 : (int) orig_process_rate );
		count = actual_process_rate - 1;
	}



	void checkRate()
	{
		if ( high_priority )
			return;

		float min_sink_node_rate = 9999999;

		for ( int i = 0; i < sinks.size(); ++i )
		{
			if ( sinks[ i ].sink_is_alive && ! sinks[ i ].node->isHidden() )
			{
				register float other_rate = sinks[ i ].node->orig_process_rate;
				if ( other_rate < min_sink_node_rate )
					min_sink_node_rate = other_rate; 							 
			}
		}

		if ( min_sink_node_rate == 9999999 || min_sink_node_rate == orig_process_rate )
			return;

		if ( ( min_sink_node_rate > orig_process_rate )		// go slow...
			 || ( min_sink_node_rate < 1.0 && min_sink_node_rate < orig_process_rate ) )	// go fast...
		{
			for ( int i = 0; i < sinks.size(); ++i )
			{
				if ( sinks[ i ].sink_is_alive )
					sinks[ i ].node->changeRate( sinks[ i ].node->orig_process_rate / min_sink_node_rate );
			}
			this->changeRate( orig_process_rate * min_sink_node_rate );
		}
	}



	~ProcessorNode()
	{
		CloseHandle( dying );
	}



	int addSource( ProcessorNode* node, Link lnk, int node_token )
	{
		assert( node );
		
		Link my_lnk( node );		

		my_lnk.copyFrom( lnk );		// copy common elements of link

		int my_token = addToLinkVector( &sources, &my_lnk );
		addToSourceDataVector( &source_buffers, new T_SOURCE_DATA(), my_token );

		sources[ my_token ].node_token = node_token;

		assert( sources.size() == source_buffers.size() );

		return my_token;
	}



	void addSink( ProcessorNode* node )
	{
		assert( node );

		Link lnk( node );

		int my_token = addToLinkVector( &sinks, &lnk );
		addToSinkDataVector( &sink_buffers, new T_SINK_DATA(), my_token );

		int other_token = node->addSource( this, lnk, my_token );

		sinks[ my_token ].node_token = other_token;

		assert( sinks.size() == sink_buffers.size() );
	}



	static int addToLinkVector( LinkList* v, Link* lnk )
	{
		assert( v );
		assert( lnk );

		int pos = v->size();
		v->resize( v->size() + 1 );

		(*v)[ pos ] = *lnk;
		
		assert( pos+1 == v->size() );

		return pos;
	}



	static void addToSourceDataVector( SourceDataList* v, T_SOURCE_DATA* buf, int pos )
	{
		assert( v );
		assert( buf );
		assert( pos >= 0 );
		assert( pos == v->size() );

		v->resize( pos +1 );

		(*v)[ pos ] = *buf;
		assert( pos+1 == v->size() );
	}



	static void addToSinkDataVector( SinkDataList* v, T_SINK_DATA* buf, int pos )
	{
		assert( v );
		assert( buf );
		assert( pos >= 0 );
		assert( pos == v->size() );

		v->resize( pos +1 );

		(*v)[ pos ] = *buf;
		assert( pos+1 == v->size() );
	}



	virtual void getCallback( T_SOURCE_DATA* d, int token )
	{
		assert( d );
 		source_buffers[ token ] = *d;
	}



	void get( T_SOURCE_DATA* d, int token )
	{
		if ( WaitForSingleObject( dying, 0 ) == WAIT_OBJECT_0 )
			return;

		assert( d );

		ResetEvent( sources[ token ].not_busy );

		getCallback( d, token );

		SetEvent( sources[ token ].received_data );
	}



	bool isDying()
	{
		return WaitForSingleObject( dying, 0 ) == WAIT_OBJECT_0;
	}



	virtual void waitForData()
	{
		int max = sources.size();

		if ( max == 1 )
		{
			DWORD rc;
			do
			{
				rc = WaitForSingleObject( sources[ 0 ].received_data, A_LONG_TIME );
				
				if ( rc == WAIT_FAILED || rc == WAIT_ABANDONED )
				{
					SetEvent( dying );
					finalise();
					terminate( RC_SOURCE_DIED );
				}

				if ( rc == WAIT_TIMEOUT )
				{
					if ( sources[ 0 ].node->getReturnCode() != STILL_ACTIVE )
					{
						SetEvent( dying );
						finalise();
						terminate( RC_SOURCE_DIED );
					}
				}
			}
			while ( rc == WAIT_TIMEOUT );
		}
		else
		{
			HANDLE* handles = new HANDLE[ max ];

			for ( int i = 0; i < max; ++i )
				handles[ i ] = sources[ i ].received_data;

			DWORD rc;
			do
			{
				rc = WaitForMultipleObjects( max, handles, true, A_LONG_TIME );
				if ( rc == WAIT_FAILED || rc == WAIT_ABANDONED )
				{
					SetEvent( dying );
					finalise();
					terminate( RC_SOURCE_DIED );
				}

				if ( rc == WAIT_TIMEOUT )
				{
					for ( int j = 0; j < sources.size(); ++j )
						if ( sources[ j ].node->getReturnCode() != STILL_ACTIVE )
						{
							SetEvent( dying );
							finalise();
							terminate( RC_SOURCE_DIED );
						}
				}
			}
			while ( rc == WAIT_TIMEOUT );

			delete[] handles;
		}
	}



	virtual void processData()
	{
		// the default processing is to pass the data unmodified.
		// this is done by copying the first source buffer to all the sink
		// node buffers.
		// This assumes that a conversion between the T_SOURCE_DATA
		// and the T_SINK_DATA class has been defined.

		if ( source_buffers.size() )
		{
			T_SOURCE_DATA* buffer = &( source_buffers[ 0 ] );

			for ( int i = 0; i < sink_buffers.size(); ++i )
				sink_buffers[ i ] = *buffer;
		}
	}



	virtual void prepareForInput()
	{
		for ( int i = 0; i < sources.size(); ++i )
		{
			source_buffers[ i ].clear();

			// this is performed last to avoid concurrent access to the source 
			// buffer above from other objects (i.e. the connected source nodes)
			SetEvent( sources[ i ].not_busy );
		}
	}



	virtual void push()
	{
#ifdef PUSH_SINK_NODES_IN_SEQUENCE

		for ( int i = 0; i < sinks.size(); ++i )
		{
			if ( sinks[ i ].sink_is_alive )
			{
				Link* lnk = &sinks[ i ];
				assert( lnk );

				if ( lnk->sink_is_alive )
				{
					T_SINK_DATA* buffer = &sink_buffers[ i ];
					assert( buffer );

					DWORD rc;

					do
					{
						rc = WaitForSingleObject( lnk->not_busy, A_LONG_TIME );

						if ( rc == WAIT_TIMEOUT )
						{
							// check if the sink node has died
							if ( lnk->node->getReturnCode() != STILL_ACTIVE )
							{
								lnk->sink_is_alive = false;
								rc = WAIT_FAILED;		// to exit loop and send data to the remaining sink nodes
							}
						}
					}
					while( rc == WAIT_TIMEOUT );

					// pass data to sink
					if ( rc == WAIT_OBJECT_0 )
						lnk->node->get( buffer, lnk->node_token );
				}
			}
		}

#else

		// we push data to the first sink node that is not busy, and then try
		// with the others, until all sink nodes have been sent the data.

		int max = sinks.size();

		if ( max == 0 )
		{
			// no sink nodes - nothing to push!
		}
		else if ( max == 1 )
		{
			Link* lnk = &sinks[ 0 ];
			assert( lnk );

			if ( lnk->sink_is_alive )
			{
				T_SINK_DATA* buffer = &sink_buffers[ 0 ];
				assert( buffer );


				DWORD rc;

				do
				{
					rc = WaitForSingleObject( lnk->not_busy, A_LONG_TIME );

					if ( rc == WAIT_TIMEOUT )
					{
						if ( lnk->node->getReturnCode() != STILL_ACTIVE )
						{
							lnk->sink_is_alive = false;
						
							rc = WAIT_FAILED;		// to exit loop and send data to the remaining sink nodes
						}
					}
				}
				while( rc == WAIT_TIMEOUT );


				// pass data to sink
				if ( rc == WAIT_OBJECT_0 )
					lnk->node->get( buffer, lnk->node_token );
			}
		}
		else
		{
			HANDLE* handles = new HANDLE[ max ];
			int*	map		= new int[ max ];
			bool*	sent	= new bool[ max ];
			int		count	= 0;

			
			for ( int i = 0; i < max; ++i )
			{
				if ( sinks[ i ].sink_is_alive )
				{
					sent[ i ] = false;
					++count;				// number of sink nodes to whom we must send data
				}
				else
					sent[ i ] = true;		// we don't send data to dead sink nodes
			}


			while( count )
			{
				count = 0;

				for ( int i = 0; i < max; ++i )
				{
					if ( ! sent[ i ] )
					{
						handles[ count ] = sinks[ i ].not_busy;
						map[ count++ ] = i;
					}
				}

				
				DWORD rc;
				do
				{
					rc = WaitForMultipleObjects( count, handles, false, A_LONG_TIME );

					if ( rc == WAIT_TIMEOUT )
					{
						// check if any of the remaining sink nodes have died
						for ( int i = 0; i < max; ++i )
							if ( ! sent[ i ] )
							{
								if ( sinks[ i ].node->getReturnCode() != STILL_ACTIVE )
								{
									sent[ i ] = true;		// we don't send data to a dead sink node
									sinks[ i ].sink_is_alive = false;
									
									rc = WAIT_FAILED;		// to exit loop and wait on the remaining sink nodes
								}
							}
					}
				}
				while ( rc == WAIT_TIMEOUT );


				if ( rc != WAIT_FAILED )
				{
					int i = map[ rc - WAIT_OBJECT_0 ];		// this is the sink node which is not busy

					Link* lnk = &sinks[ i ];
					assert( lnk );

					T_SINK_DATA* buffer = &sink_buffers[ i ];
					assert( buffer );

					// pass data to sink
					lnk->node->get( buffer, lnk->node_token );
					sent[ i ] = true;
					--count;
				}
			}

			delete[] handles;
			delete[] map;
			delete[] sent;
		}
#endif
	}



	virtual void finalise()
	{
		// here any deallocation should be done.
		// this method can be called after each step of the thread loop and
		// also when a node dies prematurely because of the death of one of
		// its source node.
	}



	virtual DWORD suspend()
	{
		safe_state.Lock();

		DWORD rc = thread->SuspendThread();

		safe_state.Unlock();

		return rc;
	}



	virtual UINT loop()
	{
		while( true )
		{
			safe_state.Lock();
			{
				prepareForInput();

				waitForData();		

				if ( ++count == actual_process_rate )
				{
					processData();
					count = 0;

					push();	
				}

				finalise();

				checkRate();
			}
			safe_state.Unlock();

			checkIfMustStop( RC_ABORTED );
		}

		return 0;	// successful
	}


	
	void dump()
	{
		cout << "----------\n[" << my_name.c_str() << "] sources = [ ";
		for ( LinkIterator i = sources.begin(); i != sources.end(); ++i )
			cout << i->node->my_name.c_str() << ":" << i->node_token << ",";
		cout << "]\n[" << my_name.c_str() << "] sinks = [ ";
		for ( i = sinks.begin(); i != sinks.end(); ++i )
			cout << i->node->my_name.c_str() << ":" << i->node_token << ",";
		cout << "]\n----------\n\n";
	}



protected:

	virtual void terminate( UINT rc )
	{
		// first wait until all sink nodes are not busy
		HANDLE* handles = new HANDLE[ sinks.size() ];

		int count, i;
		for ( count = 0, i = 0; i < sinks.size(); ++i )
		{
			if ( sinks[ i ].sink_is_alive )
				handles[ count++ ] = sinks[ i ].not_busy;
		}

		WaitForMultipleObjects( count, handles, true, INFINITE );

		delete[] handles;


		// close all sink-node handles 
		for ( i = 0; i < sinks.size(); ++i )
		{
			int b = CloseHandle( sinks[ i ].received_data );
			assert( b != 0 );
			b = CloseHandle( sinks[ i ].not_busy );
			assert( b != 0 );
		}

		
		AfxEndThread( rc );
	}

};



#endif
