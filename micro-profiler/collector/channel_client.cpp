#include "channel_client.h"

#include <atlbase.h>

namespace micro_profiler
{
	channel_t open_channel(const guid_t &id)
	{
		class frontend_stream
		{
		public:
			frontend_stream(const CLSID &id)
			{
				::CoInitialize(0);
				_frontend.CoCreateInstance(id, NULL, CLSCTX_LOCAL_SERVER);
			}

			frontend_stream(const frontend_stream &other)
				: _frontend(other._frontend)
			{	::CoInitialize(0);	}

			~frontend_stream()
			{	::CoUninitialize();	}

			bool operator()(const void *buffer, size_t size)
			{
				ULONG written;

				return _frontend && S_OK == _frontend->Write(buffer, static_cast<ULONG>(size), &written);
			}

		private:
			CComPtr<ISequentialStream> _frontend;
		};

		return frontend_stream(reinterpret_cast<const CLSID &>(id));
	}
}
