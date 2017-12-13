//	Copyright (c) 2011-2015 by Artem A. Gevorkyan (gevorkyan.org)
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
#include "object_lock.h"
#include "ProfilerMainDialog.h"
#include "symbol_resolver.h"

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
		void FinalRelease()
		{
			_dialog.reset();
			_statistics.reset();
			_symbols.reset();
		}

		void JoinTermination(signal<void()> &termination)
		{	_host_terminated = termination += bind(&disconnect, this);	}

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

				wchar_t filename[MAX_PATH] = { 0 }, extension[MAX_PATH] = { 0 };

				_wsplitpath_s(process.first.c_str(), 0, 0, 0, 0, filename, MAX_PATH, extension, MAX_PATH);
	
				_symbols = symbol_resolver::create();
				_statistics = functions_list::create(process.second, _symbols);
				_dialog.reset(new ProfilerMainDialog(_statistics, wstring(filename) + extension));
				_dialog->ShowWindow(SW_SHOW);
				_closed_connected = _dialog->Closed += bind(&disconnect, this);

				if (!_host_terminated)
					lock(_dialog);
				break;
			}

			case modules_loaded:
			{
				loaded_modules limages;

				archive(limages);
				for (loaded_modules::const_iterator i = limages.begin(); i != limages.end(); ++i)
					_symbols->add_image(i->second.c_str(), reinterpret_cast<void*>(i->first));
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
		shared_ptr<ProfilerMainDialog> _dialog;
		slot_connection _closed_connected;
		slot_connection _host_terminated;
	};

	OBJECT_ENTRY_AUTO(c_frontendClassID, ProfilerFrontend);

	shared_ptr<void> open_frontend()
	{
		class Factory : public CComClassFactory
		{
		public:
			static void Terminate(Factory *self, DWORD cookie)
			{
				self->_terminate();
				::CoRevokeClassObject(cookie);
			}

			STDMETHODIMP CreateInstance(IUnknown *outer, REFIID riid, void **object)
			{
				CComObject<ProfilerFrontend> *p = NULL;

				if (outer)
					return CLASS_E_NOAGGREGATION;
				CComObject<ProfilerFrontend>::CreateInstance(&p);
				CComPtr<IUnknown> guard(p);
				p->JoinTermination(_terminate);
				return p->QueryInterface(riid, object);
			}

		private:
			signal<void()> _terminate;
		};

		CComObject<Factory> *factory = NULL;
		DWORD cookie = 0;

		CComObject<Factory>::CreateInstance(&factory);
		CComPtr<IUnknown> guard(factory);
		if (S_OK == ::CoRegisterClassObject(c_frontendClassID, factory, CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &cookie))
			return shared_ptr<void>(factory, bind(&Factory::Terminate, _1, cookie));
		return shared_ptr<void>();
	}
}
