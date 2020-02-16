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

#include <frontend/frontend.h>

#include <frontend/function_list.h>
#include <frontend/serialization.h>
#include <frontend/symbol_resolver.h>

#include <common/memory.h>
#include <strmd/serializer.h>

using namespace std;

namespace micro_profiler
{
	frontend::frontend(ipc::channel &outbound)
		: _outbound(outbound)			
	{	}

	frontend::~frontend()
	{	released();	}

	void frontend::disconnect_session() throw()
	{	_outbound.disconnect();	}

	void frontend::disconnect() throw()
	{	}

	void frontend::message(const_byte_range payload)
	{
		unsigned id;
		buffer_reader reader(payload);
		strmd::deserializer<buffer_reader, packer> archive(reader);
		initialization_data idata;
		loaded_modules lmodules;
		module_info_metadata mmetadata;
		commands c;

		switch (archive(c), c)
		{
		case init:
			archive(idata);
			_model = functions_list::create(idata.ticks_per_second, get_resolver());
			initialized(idata.executable, _model);
			break;

		case modules_loaded:
			archive(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				get_resolver()->add_mapping(*i);
			break;

		case update_statistics:
			if (_model)
				archive(*_model);
			break;

		case module_metadata:
			archive(id);
			archive(mmetadata);
			get_resolver()->add_metadata(id, mmetadata);
			break;

		default:
			break;
		}
	}

	template <typename DataT>
	void frontend::send(commands command, const DataT &data)
	{
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);

		archive(command);
		archive(data);
		_outbound.message(const_byte_range(_buffer.data(), _buffer.size()));
	}

	shared_ptr<symbol_resolver> frontend::get_resolver()
	{
		if (!_resolver)
		{
			weak_ptr<frontend> wself = shared_from_this();

			_resolver.reset(new symbol_resolver([wself] (unsigned int persistent_id) {
				if (shared_ptr<frontend> self = wself.lock())
					self->send(request_metadata, persistent_id);
			}));
		}
		return _resolver;
	}
}
