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

#include "ProfilerSink.h"

#include "constants.h"
#include "function_list.h"
#include "ProfilerMainDialog.h"
#include "symbol_resolver.h"

#include <common/path.h>
#include <common/serialization.h>
#include <resources/resource.h>

#include <atlbase.h>
#include <atlcom.h>
#include <memory>
#include <wpl/base/signals.h>

using namespace std;
using namespace std::placeholders;
using namespace wpl;

namespace micro_profiler
{
	namespace
	{
		class buffer_reader
		{
		public:
			buffer_reader(const byte *data, size_t size)
				: _ptr(data), _remaining(size)
			{	}

			void read(void *data, size_t size)
			{
				memcpy(data, _ptr, size);
				_ptr += size;
				_remaining -= size;
			}

		private:
			const byte *_ptr;
			size_t _remaining;
		};

		void disconnect(IUnknown *object)
		{	::CoDisconnectObject(object, 0);	}

		void release(IUnknown *object)
		{	object->Release();	}

		template <typename T>
		shared_ptr<T> com_create()
		{
			CComObject<T> *p = 0;

			CComObject<T>::CreateInstance(&p);
			p->AddRef();
			return shared_ptr<T>(p, bind(&release, _1));
		}
	}

	class ATL_NO_VTABLE ProfilerFrontend
		: public ISequentialStream, public CComObjectRootEx<CComSingleThreadModel>,
			public CComCoClass<ProfilerFrontend, &c_frontendClassID>
	{
	public:
		DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERSINK)

		BEGIN_COM_MAP(ProfilerFrontend)
			COM_INTERFACE_ENTRY(ISequentialStream)
		END_COM_MAP()

	public:
		void SetOwnership(signal<void()> &termination, const get_root_window_t &root_window_cb)
		{
			_connections.push_back(termination += bind(&disconnect, this));
			_root_window_cb = root_window_cb;
		}

		STDMETHODIMP Read(void *, ULONG, ULONG *)
		{	return E_NOTIMPL;	}

		STDMETHODIMP Write(const void *message, ULONG size, ULONG *written)
		{
			buffer_reader reader(static_cast<const byte *>(message), size);
			strmd::deserializer<buffer_reader, packer> archive(reader);
			commands c;

			archive(c);
			switch (c)
			{
			case init:
			{
				initializaion_data process;

				archive(process);
				_symbols = symbol_resolver::create();
				_statistics = functions_list::create(process.second, _symbols);

				const HWND parent = _root_window_cb ? _root_window_cb() : HWND_DESKTOP;
				shared_ptr<ProfilerMainDialog> d(new ProfilerMainDialog(_statistics, *process.first, parent));

				d->ShowWindow(SW_SHOW);
				_connections.push_back(d->Closed += bind(&disconnect, this));
				lock(d);
				break;
			}

			case modules_loaded:
			{
				loaded_modules limages;

				archive(limages);
				for (loaded_modules::const_iterator i = limages.begin(); i != limages.end(); ++i)
					_symbols->add_image(i->path.c_str(), i->load_address);
				break;
			}

			case update_statistics:
				archive(*_statistics);
				break;

			case modules_unloaded:
				break;
			}
			*written = size;
			return S_OK;
		}

	private:
		shared_ptr<symbol_resolver> _symbols;
		shared_ptr<functions_list> _statistics;
		vector<slot_connection> _connections;
		get_root_window_t _root_window_cb;
	};

	OBJECT_ENTRY_AUTO(c_frontendClassID, ProfilerFrontend);

	shared_ptr<void> open_frontend_factory(const get_root_window_t &root_window_cb)
	{
		class Factory : public CComClassFactory
		{
		public:
			static shared_ptr<void> Init(const get_root_window_t &root_window_cb)
			{
				shared_ptr<Factory> f = com_create<Factory>();

				f->_root_window_cb = root_window_cb;
				if (S_OK == ::CoRegisterClassObject(c_frontendClassID, f.get(), CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &f->_cookie))
					return shared_ptr<void>(f.get(), bind(&Factory::Terminate, f.get()));
				return shared_ptr<void>();
			}

		private:
			void Terminate()
			{
				_terminate();
				::CoRevokeClassObject(_cookie);
			}

			STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **object)
			{
				if (!outer)
				{
					shared_ptr<ProfilerFrontend> p = com_create<ProfilerFrontend>();

					p->SetOwnership(_terminate, _root_window_cb);
					return p->QueryInterface(riid, object);
				}
				return CLASS_E_NOAGGREGATION;
			}

		private:
			DWORD _cookie;
			signal<void()> _terminate;
			get_root_window_t _root_window_cb;
		};

		return Factory::Init(root_window_cb);
	}
}
