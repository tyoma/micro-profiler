#include "helpers_com.h"

#include <atlbase.h>
#include <ipc/channel_client.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			namespace
			{
				class Module : public CAtlDllModuleT<Module>
				{
				public:
					Module()
					{	::CoInitialize(NULL);	}

					~Module()
					{	::CoUninitialize();	}
				} g_module;
			}

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
