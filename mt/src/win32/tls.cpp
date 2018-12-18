#include <mt/tls.h>

#include <windows.h>

namespace mt
{
	tls_base::tls_base()
		: _index(::TlsAlloc())
	{	}

	tls_base::~tls_base()
	{	::TlsFree(_index);	}

	void *tls_base::get() const
	{	return ::TlsGetValue(_index);	}

	void tls_base::set(void *value)
	{	::TlsSetValue(_index, value);	}
}
