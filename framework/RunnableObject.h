
#ifndef RUNNABLE_OBJECT_H
#define RUNNABLE_OBJECT_H


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <afxwin.h>
#include <assert.h>



class RunnableObject
{
protected:

	CWinThread* thread;
	bool do_stop;
	bool has_started;

	
	enum {	RC_SUCCESS		= 0,
			RC_ABORTED		= 8,
			RC_ERROR		= 9,
			RC_SOURCE_DIED	= 10 };


	enum { A_LONG_TIME = 500 };


public:
	
	RunnableObject()
		: thread( 0 )
	{
		thread = AfxBeginThread( RunnableObject::ThreadFn, 
								 this,
								 THREAD_PRIORITY_NORMAL,
								 0,								// default stack size
								 CREATE_SUSPENDED );
		assert( thread );

		// set to FALSE so that the thread object is not deleted on thread termination,
		// so that we could still interrogate the threa object for stuff like the
		// return code value, etc.
		thread->m_bAutoDelete = FALSE;
		
		do_stop = false;
		has_started = false;
	}



	HANDLE handle()
	{
		assert( thread );
		return thread->m_hThread;
	}



	// please note that the thread ID can be re-cycled by Windows, once
	// a thread dies.
	DWORD threadID()
	{
		assert( thread );
		return thread->m_nThreadID;
	}



	virtual DWORD resume()
	{
		has_started = true;
		return thread->ResumeThread();
	}



	virtual DWORD suspend()
	{
		return thread->SuspendThread();
	}



	bool hasStarted()
	{
		return has_started;
	}



	DWORD end( DWORD block_time = 0 )
	{
		do_stop = true;

		return wait( block_time );
	}



	DWORD wait( DWORD block_time = 0 )
	{
		if ( block_time && has_started )
			::WaitForSingleObject( thread->m_hThread, block_time );

		return getReturnCode();
	}



	DWORD getReturnCode()
	{
		DWORD rc;
		GetExitCodeThread( thread->m_hThread, &rc );

		return rc;
	}



	virtual ~RunnableObject()
	{
		// in case thread is still running...
		end( INFINITE );		

		// thread object auto-deletion on thread termination has been 
		// disabled - so we must delete the object manually.
		assert( thread );
		delete thread;
	}	



	static UINT ThreadFn( LPVOID pParam )
	{
		assert( pParam );
		
		RunnableObject* obj = (RunnableObject*) pParam;	
		assert( obj );

		return obj->loop();
	}



	virtual UINT loop()
	{
		// more interesting processing is usually done here, and
		// in each loop we must check if the thread has been 
		// terminated from outside. checkIfMustStop() should be called at
		// a safe place from within the loop.
		checkIfMustStop( RC_ABORTED );

		return RC_SUCCESS;
	}



	virtual void terminate( UINT rc )
	{
		AfxEndThread( rc );
	}



protected:

	virtual void checkIfMustStop( UINT rc )
	{
		if ( do_stop )
			terminate( rc );
	}

};



#endif

