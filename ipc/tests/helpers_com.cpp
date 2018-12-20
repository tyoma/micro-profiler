#include "helpers_com.h"

#include <ipc/channel_client.h>
#include <ipc/com/endpoint.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace com
		{
			shared_ptr<session_factory> channel_factory::create_default_session_factory()
			{	return shared_ptr<session_factory>();	}
		}

		namespace tests
		{
			namespace
			{
				class Module : public CAtlDllModuleT<Module> {	} g_module;
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
			{	return open_channel(id);	}
		}
	}
}
