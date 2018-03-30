#include "frontend.h"

#include "function_list.h"
#include "symbol_resolver.h"

#include <common/serialization.h>

using namespace std;

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
	}

	Frontend::Frontend()
		: _resolver(symbol_resolver::create())
	{	}

	void Frontend::Disconnect()
	{	::CoDisconnectObject(this, 0);	}

	void Frontend::FinalRelease()
	{	released();	}

	STDMETHODIMP Frontend::Read(void *, ULONG, ULONG *)
	{	return E_NOTIMPL;	}

	STDMETHODIMP Frontend::Write(const void *message, ULONG size, ULONG * /*written*/)
	{
		buffer_reader reader(static_cast<const byte *>(message), size);
		strmd::deserializer<buffer_reader, packer> archive(reader);
		initialization_data idata;
		loaded_modules lmodules;
		commands c;

		archive(c);
		switch (c)
		{
		case init:
			archive(idata);
			_model = functions_list::create(idata.second, _resolver);
			initialized(idata.first, _model);
			break;

		case modules_loaded:
			archive(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				_resolver->add_image(i->path.c_str(), i->load_address);
			break;

		case update_statistics:
			archive(*_model);
			break;
		}
		return S_OK;
	}
}
