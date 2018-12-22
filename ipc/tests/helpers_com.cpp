#include "helpers_com.h"

#include <ipc/com/endpoint.h>
#include <ut/assert.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			shared_ptr<ipc::server> server::create_default_session_factory()
			{	return shared_ptr<ipc::server>();	}
		}

		namespace tests
		{
			namespace
			{
				class Module : public CAtlDllModuleT<Module> {	} g_module;

				class sequential_stream
				{
				public:
					sequential_stream(const CLSID &id)
						: _frontend(0)
					{
						::CoCreateInstance(id, NULL, CLSCTX_LOCAL_SERVER, IID_ISequentialStream, (void **)&_frontend);
						assert_not_null(_frontend);
					}

					sequential_stream(const sequential_stream &other)
						: _frontend(other._frontend)
					{	_frontend->AddRef();	}

					~sequential_stream()
					{	_frontend->Release();	}

					bool operator()(const void *buffer, size_t size)
					{
						ULONG written;

						return S_OK == _frontend->Write(buffer, static_cast<ULONG>(size), &written);
					}

				private:
					ISequentialStream *_frontend;
				};

			}


			com_initialize::com_initialize()
			{	::CoInitialize(NULL);	}

			com_initialize::~com_initialize()
			{	::CoUninitialize();	}


			bool is_factory_registered(const guid_t &id)
			{
				void *p = 0;

				if (S_OK == ::CoGetClassObject(reinterpret_cast<const GUID &>(id), CLSCTX_LOCAL_SERVER, NULL,
					IID_IClassFactory, &p) && p)
				{
					static_cast<IUnknown *>(p)->Release();
					return true;
				}
				return false;
			}

			stream_function_t open_stream(const guid_t &id)
			{	return sequential_stream(reinterpret_cast<const CLSID &>(id));	}
		}
	}
}
