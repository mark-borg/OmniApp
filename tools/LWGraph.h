
#ifndef LW_GRAPH_H
#define LW_GRAPH_H


#include <iostream>
#include "LWList.h"



template< class LIST_A, class LIST_B >
class LWGraph
{

	// usage m[ <list_B_index> ][ <list_A_index ]
	bool** m;
	
	LIST_A* a;
	LIST_B* b;

	int a_size;
	int b_size;


public:

	
	LWGraph( LIST_A* list_a, LIST_B* list_b )
	{
		a = list_a;
		b = list_b;

		a_size = list_a->capacity();
		b_size = list_b->capacity();


		typedef bool* bool_ptr;
		m = new bool_ptr[ b_size ];
		for ( int i = 0; i < b_size; ++i )
			m[ i ] = new bool[ a_size ];


		// reset all links
		for ( int ai = 0; ai < a_size; ++ai )
			for ( int bi = 0; bi < b_size; ++bi )
				m[ bi ][ ai ] = false;

		assert( a );
		assert( b );
	}


	~LWGraph()
	{
		for ( int i = 0; i < b_size; ++i )
			delete[] m[ i ];
		delete[] m;
	}



	void link( int index_a, int index_b )
	{
		assert( index_a >= 0 && index_a < a_size );
		assert( index_b >= 0 && index_b < b_size );

		assert( ! a->isFree( index_a ) );
		assert( ! b->isFree( index_b ) );

		assert( m[ index_b ][ index_a ] == false );
		m[ index_b ][ index_a ] = true;
	}


	void unlink( int index_a, int index_b )
	{
		assert( index_a >= 0 && index_a < a_size );
		assert( index_b >= 0 && index_b < b_size );

		assert( ! a->isFree( index_a ) );
		assert( ! b->isFree( index_b ) );

		assert( m[ index_b ][ index_a ] == true );
		m[ index_b ][ index_a ] = false;
	}


	bool isLinked( int index_a, int index_b )
	{
		assert( index_a >= 0 && index_a < a_size );
		assert( index_b >= 0 && index_b < b_size );
		
		assert( ! a->isFree( index_a ) );
		assert( ! b->isFree( index_b ) );

		return m[ index_b ][ index_a ];
	}


	void selfLink()
	{
		assert( a_size == b_size );

		for ( int ai = 0; ai < a_size; ++ai )
		{
			if ( ! a->isFree( ai ) )
				m[ ai ][ ai ] = true;
		}
	}


	int numLinksA( int index_a )
	{
		int cnt = 0;

		for ( int y = firstLinkA( index_a ); y != -1; y = nextLinkA( index_a, y ) )
			++cnt;

		return cnt;
	}


	int numLinksB( int index_b )
	{
		int cnt = 0;

		for ( int x = firstLinkB( index_b ); x != -1; x = nextLinkB( index_b, x ) )
			++cnt;

		return cnt;
	}


	int firstLinkA( int index_a )
	{
		assert( index_a >= 0 && index_a < a_size );	
		assert( ! a->isFree( index_a ) );

		for ( int x = 0; x < b_size; ++x )
		{
			if ( m[ x ][ index_a ] )
				return x;
		}

		return -1;
	}


	int nextLinkA( int index_a, int prev_index_b )
	{
		assert( index_a >= 0 && index_a < a_size );	
		assert( ! a->isFree( index_a ) );

		++prev_index_b;

		for ( int x = prev_index_b; x < b_size; ++x )
		{
			if ( m[ x ][ index_a ] )
				return x;
		}

		return -1;
	}


	int firstLinkB( int index_b )
	{
		assert( index_b >= 0 && index_b < b_size );	
		assert( ! b->isFree( index_b ) );

		for ( int y = 0; y < a_size; ++y )
		{
			if ( m[ index_b ][ y ] )
				return y;
		}

		return -1;
	}


	int nextLinkB( int index_b, int prev_index_a )
	{
		assert( index_b >= 0 && index_b < b_size );	
		assert( ! b->isFree( index_b ) );

		++prev_index_a;

		for ( int y = prev_index_a; y < a_size; ++y )
		{
			if ( m[ index_b ][ y ] )
				return y;
		}

		return -1;
	}


	void dump( bool compact = true )
	{
		cout << "-----------------------------\n";

		cout << "   ";
		for ( int x = 0; x < b_size; ++x )
			if ( ! b->isFree( x ) )
				cout << x / 10;
		cout << "\n   ";
		for ( x = 0; x < b_size; ++x )
			if ( ! b->isFree( x ) )
				cout << x % 10;
		cout << "\n";

		for ( int y = 0; y < a_size; ++y )
		{
			if ( ! a->isFree( y ) )
			{
				cout << y / 10 << y % 10 << " ";
				for ( int x = 0; x < b_size; ++x )
				{
					if ( ! b->isFree( x ) )
					{
						if ( m[ x ][ y ] )
							cout << "x";
						else
							cout << ".";
					}
				}
				cout << "\n";
			}
		}
				
		cout << "-----------------------------\n";
	}

};




#endif

