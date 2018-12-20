//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <ipc/com/endpoint.h>

#include <common/string.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			class endpoint : public ipc::endpoint
			{
				virtual shared_ptr<session_factory> create_active(const char *destination_endpoint);
				virtual shared_ptr<void> create_passive(const char *endpoint_id, const shared_ptr<session_factory> &sf);
			};


			void channel::FinalRelease()
			{	inbound->disconnect();	}

			STDMETHODIMP channel::Read(void *, ULONG, ULONG *)
			{	return E_UNEXPECTED;	}

			STDMETHODIMP channel::Write(const void *message, ULONG size, ULONG * /*written*/)
			{
				inbound->message(const_byte_range(static_cast<const byte *>(message), size));
				return S_OK;
			}

			void channel::disconnect()
			{	::CoDisconnectObject(this, 0);	}

			void channel::message(const_byte_range /*payload*/)
			{	}


			channel_factory::channel_factory(const shared_ptr<session_factory> &factory)
				: _session_factory(factory)
			{	}

			void channel_factory::set_session_factory(const shared_ptr<session_factory> &factory)
			{	_session_factory = factory;	}

			STDMETHODIMP channel_factory::CreateInstance(IUnknown * /*outer*/, REFIID riid, void **object)
			try
			{
				CComObject<channel> *p = 0;
				CComObject<channel>::CreateInstance(&p);
				CComPtr<ISequentialStream> lock(p);

				p->inbound = _session_factory->create_session(*p);
				return p->QueryInterface(riid, object);
			}
			catch (const bad_alloc &)
			{
				return E_OUTOFMEMORY;
			}


			shared_ptr<session_factory> endpoint::create_active(const char * /*destination_endpoint*/)
			{
				throw 0;
			}

			shared_ptr<void> endpoint::create_passive(const char *endpoint_id, const shared_ptr<session_factory> &sf)
			{
				guid_t id = from_string(endpoint_id);
				const CLSID &clsid = reinterpret_cast<const CLSID &>(id);
				CComObject<channel_factory> *p = 0;

				CComObject<channel_factory>::CreateInstance(&p);

				CComPtr<IClassFactory> lock(p);
				DWORD cookie;

				p->set_session_factory(sf);
				if (S_OK == ::CoRegisterClassObject(clsid, p, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie))
					return shared_ptr<void>(p, bind(&::CoRevokeClassObject, cookie));
				throw 0;
			}


			shared_ptr<ipc::endpoint> create_endpoint()
			{	return shared_ptr<endpoint>(new endpoint);	}
		}
	}
}
