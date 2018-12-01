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

#include <frontend/frontend.h>

#include <frontend/function_list.h>

#include <common/memory.h>
#include <common/serialization.h>
#include <common/symbol_resolver.h>

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
				mem_copy(data, _ptr, size);
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

	void Frontend::disconnect() throw()
	{	::CoDisconnectObject(this, 0);	}

	void Frontend::FinalRelease()
	{
		if (_model)
			_model->release_resolver();
		released();
	}

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
			_model = functions_list::create(idata.ticks_per_second, _resolver);
			initialized(idata.executable, _model);
			break;

		case modules_loaded:
			archive(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				_resolver->add_image(i->path.c_str(), i->load_address);
			break;

		case update_statistics:
			if (_model)
				archive(*_model);
			break;
		}
		return S_OK;
	}
}
