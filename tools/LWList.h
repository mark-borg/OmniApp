
#ifndef LW_LIST_H
#define LW_LIST_H


#include <iostream>
#include <assert.h>



#define DONT_REUSE_DELETED


 

template< class ELEMENT >
class LWList
{
	int cur_capacity;
	int grow_by;

	ELEMENT* e;
	bool* free;
	int num;

#ifdef DONT_REUSE_DELETED
	int max_index;
#endif


public:

	
	typedef class ELEMENT	T_ELEMENT;

	

	LWList( int initial_capacity, int grow_by = 0 )
	{
		cur_capacity = initial_capacity;
		this->grow_by = grow_by;

		e = new ELEMENT[ initial_capacity ];
		free = new bool[ initial_capacity ];

		for ( int i = 0; i < initial_capacity; ++i )
			free[ i ] = true;

		num = 0;
	
#ifdef DONT_REUSE_DELETED
		max_index = -1;
#endif
	}


	~LWList()
	{
		delete[] e;
		delete[] free;
	}


	int capacity()
	{
		return cur_capacity;
	}


	int size()
	{
		return num;
	}


	ELEMENT* getAt( int index )
	{
		assert( index >= 0 && index < cur_capacity );
		
		if ( free[ index ] )
			return 0;

		return &e[ index ];
	}


	int add( const ELEMENT& a )
	{
		int next_free = getNextFreeIndex();
		assert( next_free != -1 );

		e[ next_free ] = a;
		free[ next_free ] = false;
		++num;

#ifdef DONT_REUSE_DELETED
		max_index = max_index > next_free ? max_index : next_free;
#endif

		return next_free;
	}


	int add( int index, const ELEMENT& a )
	{
		assert( index >= 0 );
		
		if ( index >= cur_capacity && grow_by )
		{
			int grow_by_amount = grow_by;
			while( index > ( cur_capacity + grow_by_amount ) )
				grow_by_amount += grow_by;
			
			regrow( grow_by_amount );
		}

		assert( index < cur_capacity );
		assert( free[ index ] == true );

		e[ index ] = a;
		free[ index ] = false;
		++num;

#ifdef DONT_REUSE_DELETED
		max_index = MAX( max_index, index );
#endif

		return index;
	}
	void remove( int index )
	{
		assert( index >= 0 && index < cur_capacity );
		assert( ! free[ index ] );
		
		free[ index ] = true;
		--num;
	}


	int getNextFreeIndex()
	{
		int first_free = -1;

#ifdef DONT_REUSE_DELETED
		for ( int i = max_index+1; i < cur_capacity && first_free == -1; ++i )
#else
		for ( int i = 0; i < cur_capacity && first_free == -1; ++i )
#endif
		{
			if ( free[ i ] )
				first_free = i;
		}

		if ( first_free == -1 && grow_by )
			return regrow( grow_by );

		return first_free;
	}


	int first()
	{
		for ( int i = 0; i < cur_capacity; ++i )
			if ( ! free[ i ] )
				return i;

		return -1;
	}


	int next( int prev_index )
	{
		++prev_index;

		for ( int i = prev_index; i < cur_capacity; ++i )
			if ( ! free[ i ] )
				return i;

		return -1;
	}


	bool isFree( int index )
	{
		assert( index >= 0 && index < cur_capacity );
		
		return free[ index ];
	}


	void dump( bool compact = true )
	{
		using namespace std;

		cout << "-----------------------------\n";
		cout << "size = " << size() << " / " << capacity() << "\n";

		for ( int i = 0; i < capacity(); ++i )
		{
			if ( free[ i ] )
			{
				if ( ! compact )
					cout << i << "\t: " << "empty\n";
			}
			else
			{
				cout << i << "\t: " << (void*) &e[ i ] << "  [" << e[ i ] << "]\n";
			}
		}

		cout << "-----------------------------\n";
	}



protected:


	int regrow( int grow_by_amount )
	{
		assert( grow_by_amount > 0 );

		// reallocate
		ELEMENT* ee = new ELEMENT[ cur_capacity + grow_by_amount ];
		bool* ff = new bool[ cur_capacity + grow_by_amount ];
		
		assert( ee );
		assert( ff );

		for ( int k = 0; k < cur_capacity; ++k )
		{
			ee[ k ] = e[ k ];
			ff[ k ] = free[ k ];
		}
		for ( int k = 0; k < grow_by_amount; ++k )
		{
			ff[ cur_capacity + k ] = true;
		}

		delete[] e;
		delete[] free;

		e = ee;
		free = ff;

		int k = cur_capacity;
		cur_capacity += grow_by_amount;
		
		return k;
	}

};




#endif

