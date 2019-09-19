//	Copyright (c) 2011-2019 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <ipc/endpoint_com.h>

#include <atlbase.h>
#include <atlcom.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			class session : public ISequentialStream, public /*outbound*/ ipc::channel,
				public CComObjectRootEx<CComSingleThreadModel>
			{
			public:
				BEGIN_COM_MAP(session)
					COM_INTERFACE_ENTRY(ISequentialStream)
				END_COM_MAP()

				void FinalRelease();

			public:
				shared_ptr<ipc::channel> inbound;

			private:
				STDMETHODIMP Read(void *, ULONG, ULONG *);
				STDMETHODIMP Write(const void *message, ULONG size, ULONG *written);

				virtual void disconnect() throw();
				virtual void message(const_byte_range payload);
			};


			class server : public CComClassFactory
			{
			public:
				server(const std::shared_ptr<ipc::server> &factory = create_default_session_factory());

				void set_server(const std::shared_ptr<ipc::server> &factory);

				static std::shared_ptr<ipc::server> create_default_session_factory();

			private:
				STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **object);

			private:
				shared_ptr<ipc::server> _session_factory;
			};
		}
	}
}
