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

#include <frontend/frontend_manager.h>

#include <frontend/function_list.h>

#include <common/memory.h>
#include <common/serialization.h>
#include <common/symbol_resolver.h>

using namespace std;

namespace micro_profiler
{
	frontend::frontend(ipc::channel &outbound)
		: _outbound(outbound), _resolver(symbol_resolver::create())
	{	}

	frontend::~frontend()
	{
		if (_model)
			_model->release_resolver();
		released();
	}

	void frontend::disconnect() throw()
	{	}

	void frontend::message(const_byte_range payload)
	{
		buffer_reader reader(payload);
		strmd::deserializer<buffer_reader, packer> archive(reader);
		initialization_data idata;
		loaded_modules lmodules;
		commands c;

		switch (archive(c), c)
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

		default:
			break;
		}
	}

	void frontend::disconnect_session() throw()
	{	_outbound.disconnect();	}
}
