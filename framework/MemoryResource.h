
#ifndef MEMORY_RESOURCE_H
#define MEMORY_RESOURCE_H




template< class RES_TYPE >
class MemoryResource
{
public:

	// resource type specifics
	RES_TYPE type;

	// resource usage indicator
	int use_count;

	// resource pointer
	unsigned long id;



	bool free() const
	{
		return use_count == 0;
	}



	int in_use() const
	{
		return use_count == 0 ? 0 : 1;
	}



	MemoryResource( RES_TYPE type, typename RES_TYPE::ptr_type ptr )
	{
		this->type = type;

		use_count = 0;

		id = (unsigned long) ptr;
	}



	typename RES_TYPE::ptr_type ptrToData() const
	{
		void* ptr = (void*) id;
		return (RES_TYPE::ptr_type) ptr;
	}	



	long memoryUsed()
	{
		return type.memoryUsed();
	}
};



template< class RES_TYPE >
bool operator< ( const MemoryResource< RES_TYPE >& a, 
				 const MemoryResource< RES_TYPE >& b )
{
	int res = a.type.compare( b.type );

	if ( res == -1 )
		return true;
	else if ( res == 1 ) 
		return false;

	if ( a.in_use() < b.in_use() ) 
		return true;
	else if ( a.in_use() > b.in_use() ) 
		return false;

	return ( a.id < b.id );
}




#endif

