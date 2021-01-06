//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <functional>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			namespace
			{
				template <typename T>
				CComPtr< CComObject<T> > construct()
				{
					CComObject<T> *p;

					if (E_OUTOFMEMORY == CComObject<T>::CreateInstance(&p) || !p)
						throw bad_alloc();
					return CComPtr< CComObject<T> > (p);
				}
			}



			void session::FinalRelease()
			{	inbound->disconnect();	}

			STDMETHODIMP session::Read(void *, ULONG, ULONG *)
			{	return E_UNEXPECTED;	}

			STDMETHODIMP session::Write(const void *message, ULONG size, ULONG * /*written*/)
			{
				inbound->message(const_byte_range(static_cast<const byte *>(message), size));
				return S_OK;
			}

			STDMETHODIMP session::GetConnectionInterface(IID *iid)
			{	return iid ? *iid = IID_ISequentialStream, S_OK : E_POINTER;	}

			STDMETHODIMP session::GetConnectionPointContainer(IConnectionPointContainer ** /*container*/)
			{	return E_NOTIMPL;	}

			STDMETHODIMP session::Advise(IUnknown *sink, DWORD *cookie)
			{
				if (!cookie)
					return E_POINTER;
				if (!sink)
					return E_INVALIDARG;
				if (_outbound)
					return CONNECT_E_ADVISELIMIT;

				HRESULT hr = sink->QueryInterface(&_outbound);

				if (S_OK == hr)
				{
					*cookie = 1;
				}
				return hr;
			}

			STDMETHODIMP session::Unadvise(DWORD cookie)
			{	return cookie == 1 && _outbound ? _outbound = 0, S_OK : CONNECT_E_NOCONNECTION;	}

			STDMETHODIMP session::EnumConnections(IEnumConnections ** /*enumerator*/)
			{	return E_NOTIMPL;	}

			void session::disconnect()
			{	::CoDisconnectObject(static_cast<ISequentialStream *>(this), 0);	}

			void session::message(const_byte_range payload)
			{
				if (!_outbound)
					throw runtime_error("Sink channel has not been created yet!");
				_outbound->Write(payload.begin(), static_cast<ULONG>(payload.length()), NULL);
			}


			server::server(const shared_ptr<ipc::server> &factory)
				: _session_factory(factory)
			{	}

			void server::set_server(const shared_ptr<ipc::server> &factory)
			{	_session_factory = factory;	}

			STDMETHODIMP server::CreateInstance(IUnknown * /*outer*/, REFIID riid, void **object)
			try
			{
				CComPtr< CComObject<session> > p = construct<session>();

				p->inbound = _session_factory->create_session(*p);
				return p->QueryInterface(riid, object);
			}
			catch (const bad_alloc &)
			{
				return E_OUTOFMEMORY;
			}


			shared_ptr<void> run_server(const char *endpoint_id, const shared_ptr<ipc::server> &sf)
			{
				guid_t id = from_string(endpoint_id);
				const CLSID &clsid = reinterpret_cast<const CLSID &>(id);
				CComPtr< CComObject<server> > p = construct<server>();
				DWORD cookie;

				p->set_server(sf);
				if (S_OK == ::CoRegisterClassObject(clsid, p, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie))
					return shared_ptr<void>((IUnknown *)p, bind(&::CoRevokeClassObject, cookie));
				throw initialization_failed(endpoint_id);
			}
		}
	}
}
