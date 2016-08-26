
#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H


#pragma warning( disable : 4786 )


#include <map>
#include <iomanip>
#include <assert.h>
#include <afxmt.h>				// multithreading stuff
#include "MemoryResource.h"

using namespace std;



#ifndef LOG
#define LOG(A)
#endif




template< class RES_TYPE >
class MemoryPool
{
public:

	typedef typename RES_TYPE::ptr_type			ptr_type;
	typedef MemoryResource< RES_TYPE >			RES;
	typedef map< RES, RES >						mempool_type;
	typedef typename map< RES, RES >::iterator	mempool_iterator;
	typedef pair< RES, RES >					mempool_pair;
	typedef pair< mempool_iterator, bool >		mempool_rc;
	typedef typename mempool_type::size_type	size_type;

	
	mempool_type pool;

	
	CCriticalSection cs;



	ptr_type acquire( RES_TYPE type )
	{
		ptr_type ptr;

		cs.Lock();
		{
			RES r( type, 0 );

			mempool_iterator i = pool.lower_bound( r );

			if ( i != pool.end() )
				if ( (*i).second.free() )
					if ( (*i).second.type == type )
					{
						RES s = (*i).second;

						// mark resource as 'in use'
						int rc = pool.erase( s );
						assert( rc == 1 );

						++s.use_count;
						assert( s.use_count == 1 );

						mempool_rc rd = pool.insert( mempool_pair( s, s ) );
						assert( rd.second );


						cs.Unlock();
						return s.ptrToData();
					}

			ptr = type.allocate();
			assert( ptr );

			// add new resource to pool and mark it as 'in use'
			RES s( type, ptr );
			
			++s.use_count;
			
			mempool_rc rd = pool.insert( mempool_pair( s, s ) );
			assert( rd.second );
		}
		cs.Unlock();

		return ptr;
	}



	ptr_type acquireClean( RES_TYPE type )
	{
		ptr_type ptr = acquire( type );
		return type.clear( ptr );
	}

	

	void release( ptr_type ptr )
	{
		cs.Lock();
		{
			RES_TYPE type = RES_TYPE::getType( ptr );

			RES r( type, ptr );
			r.use_count = 1;			// object is in use at least once

			mempool_iterator i = pool.find( r );
			assert( i != pool.end() );
			
			RES s = (*i).second;
			assert( ! s.free() );


			// decrease usage count
			int rc = pool.erase( s );
			assert( rc == 1 );

			--s.use_count;

			mempool_rc rd = pool.insert( mempool_pair( s, s ) );
			assert( rd.second );
		}
		cs.Unlock();

		ptr = 0;
		assert( ptr == 0 );
	}



	void use( ptr_type ptr )
	{
		cs.Lock();
		{
			RES_TYPE type = RES_TYPE::getType( ptr );
			RES r( type, ptr );
			r.use_count = 1;			// object is in use at least once

			mempool_iterator i = pool.find( r );

			assert( i != pool.end() );
			
			RES s = (*i).second;
			assert( ! s.free() );

			// increase usage count
			int rc = pool.erase( s );
			assert( rc == 1 );

			++s.use_count;

			mempool_rc rd = pool.insert( mempool_pair( s, s ) );
			assert( rd.second );
		}
		cs.Unlock();
	}



	void dump()
	{
		LOG( "" );
		LOG( "MEMORY IMAGE POOL DUMP" );
		LOG( "----------------------------------------------------------" );

		long total_memory = 0;

		cs.Lock();
		{
			int a = 0;

			for ( mempool_iterator i = pool.begin(); i != pool.end(); ++i )
			{
				RES r = (*i).second;
				LOG( ++a << "\ttype[ " << r.type << " ]\tuse count: " <<
					 r.use_count << "\tptr: " << (ptr_type) r.id );

				total_memory += r.memoryUsed();
			}
		}
		cs.Unlock();

		total_memory /= 1048576.0;

		LOG( "" );
		LOG( "total memory pool size = " << setprecision(2) << setiosflags( ios::fixed ) << total_memory << " Mb." );
		LOG( "----------------------------------------------------------" );
	}



	size_type numResources() 
	{
		return pool.size();
	}


	
	size_type numFreeResources()
	{
		size_type n = 0;

		cs.Lock();
		{
			for ( mempool_iterator i = pool.begin(); i != pool.end(); ++i )
			{
				RES r = (*i).second;
				n += r.use_count == 0 ? 1 : 0;
			}
		}
		cs.Unlock();

		return n;
	}



	~MemoryPool()
	{
		cs.Lock();
		{
			for ( mempool_iterator i = pool.begin(); i != pool.end(); ++i )
			{
				RES r = (*i).second;
				r.type.deallocate( r.ptrToData() );
			}
	
			pool.clear();
		}
		cs.Unlock();
	}

};




#endif

