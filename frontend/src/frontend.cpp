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

#include <common/memory.h>
#include <common/serialization.h>
#include <common/symbol_resolver.h>
#include <strmd/serializer.h>

using namespace std;

namespace micro_profiler
{
	frontend::frontend(ipc::channel &outbound)
		: _outbound(outbound), _resolver(new symbol_resolver())
	{	}

	frontend::~frontend()
	{
		released();
	}

	void frontend::disconnect_session() throw()
	{	_outbound.disconnect();	}

	void frontend::disconnect() throw()
	{	}

	void frontend::message(const_byte_range payload)
	{
		buffer_reader reader(payload);
		strmd::deserializer<buffer_reader, packer> archive(reader);
		initialization_data idata;
		loaded_modules lmodules;
		module_info_basic mbasic;
		module_info_metadata mmetadata;
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
				send(request_metadata, *i);
			break;

		case update_statistics:
			if (_model)
				archive(*_model);
			break;

		case module_metadata:
			archive(mbasic);
			archive(mmetadata);
			_resolver->add_metadata(mbasic, mmetadata);
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
}
