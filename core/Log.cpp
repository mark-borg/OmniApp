
#include "log.h"
#include "..\gui\stdafx.h"
#include "..\gui\OmniApp2.h"



Log::Log()
{
	theApp = (CMainApp*) AfxGetApp();
	assert( theApp );
}



ostream& Log::operator() ( )
{
	clear();
	return buffer;
}



void Log::dump( unsigned long colour )
{
	buffer << ends;
	assert( buffer.good() );

	if ( static_cast<CMainApp*>( theApp )->log_view )
		static_cast<CMainApp*>( theApp )->log_view->AddMessage( buffer.str(), static_cast<COLORREF>( colour ) );

	// unfreeze the buffer
	buffer.rdbuf()->freeze( 0 );
}



void Log::clear()
{
	// rewind pointers
	buffer.seekp( 0 );
	buffer.seekg( 0 );

	// clear any error states
	buffer.clear();

	assert( buffer.good() );
}




// the Log instance(s)
Log LogOne;


