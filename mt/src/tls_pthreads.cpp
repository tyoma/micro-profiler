#include <mt/tls.h>

#include <new>
#include <pthread.h>

namespace mt
{
	namespace
	{
		void dummy_dtor(void*)
		{	}
	}
	
	tls_base::tls_base()
	{
		pthread_key_t key;
		
		if (::pthread_key_create(&key, &dummy_dtor))
			throw std::bad_alloc();
		_index = key;
	}

	tls_base::~tls_base()
	{	::pthread_key_delete(_index);	}

	void *tls_base::get() const
	{	return ::pthread_getspecific(_index);	}

	void tls_base::set(void *value)
	{	::pthread_setspecific(_index, value);	}
}
