
#ifndef SINGLETON_DESTROYER_H
#define SINGLETON_DESTROYER_H



template<class T>
class SingletonDestroyer
{
public:

	SingletonDestroyer()
	: m_destroy(0)
	{ }


	~SingletonDestroyer();


	void setForDestruction( T* ptr )
	{
		m_destroy = ptr;
	}


private:

	T* m_destroy;

};



template<class T>
SingletonDestroyer<T>::~SingletonDestroyer()
{
	if ( m_destroy )
		delete m_destroy;

	m_destroy = 0;
}



#endif

