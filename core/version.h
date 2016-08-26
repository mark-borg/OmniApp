
#ifndef VERSION_H
#define VERSION_H


#include "..\tools\singleton.h"




class Version : public Singleton< Version >
{
private:

	enum { MAX_LINE = 100 };

	
	char build[ MAX_LINE ];
	char version[ MAX_LINE ];
	char libraries[ MAX_LINE * 4 ];



public:

	const char* getBuild()
	{
		return build;
	}



	const char* getVersion()
	{
		return version;
	}



	const char* getLibraries()
	{
		return libraries;
	}



protected:

	// Version cannot be constructed directly because it is a 
	// Singleton class - the singleton must be obtained through
	// calling static member function Instance()

	Version();



	virtual ~Version()
	{
	}



	// since we made ctor and dtor protected, we must declare the
	// following template classes as friends.  Singleton<A> will need
	// to call the constructor to create the one and only instance;
	// while SingletonDestroyer<A> will need to call the destructor
	// when destroying the one and only instance.
	friend class Singleton<Version>;
	friend class SingletonDestroyer<Version>;

};



#endif
