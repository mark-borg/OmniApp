
#ifndef SINGLETON_H
#define SINGLETON_H



#include "SingletonDestroyer.h"



template<class T>
class Singleton
{
public:

	typedef T* 	T_ptr;
	typedef T  	T_type;


	inline static
	T_ptr instance();


	inline static
	void killInstance();


protected:

	typedef SingletonDestroyer<T>	T_destroyer;

	
	Singleton()				// can only be inherited from
	{ }


	static 
	void createInstance();


	// Prevent users from making copies of a Singleton
	Singleton( const Singleton<T>& );

	Singleton<T>& operator= ( const Singleton<T>& );


	// the one and only...
	static T_ptr m_instance;
};



template<class T>
typename Singleton<T>::T_ptr Singleton<T>::m_instance;



template<class T> 
inline 
typename Singleton<T>::T_ptr Singleton<T>::instance()
{
	if ( ! m_instance )
		createInstance();

	return m_instance;
}



template<class T> 
inline
void Singleton<T>::killInstance()
{
	if ( m_instance )
		delete m_instance;

	m_instance = 0;
}



template<class T> 
void Singleton<T>::createInstance()
{
	// ideally, the destroyer object should be declared as a class
	// static, but some compilers have problems with static members 
	// of templates. This will give rise to erros when defining the
	// static data of the template.
	static T_destroyer destroyer;


	m_instance = new T_type;

	// pass ownership for destruction of the singleton instance
	// to this function's static SingletonDestroyer class
	destroyer.setForDestruction( m_instance );
}


// CreateInstance() should not be inlined, else its static 
// variable destroyer will be duplicated due to the internal 
// linkage done by the compiler on inline functions.




#endif
