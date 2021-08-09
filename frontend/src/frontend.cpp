//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <frontend/image_patch_model.h>
#include <frontend/serialization.h>
#include <frontend/symbol_resolver.h>

#include <common/memory.h>
#include <logger/log.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>

#define PREAMBLE "Frontend: "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		const mt::milliseconds c_updateInterval(25);
	}

	frontend::frontend(ipc::channel &outbound, shared_ptr<scheduler::queue> queue)
		: _outbound(outbound), _queue(queue), _alive(make_shared<bool>(true))
	{
		auto alive = _alive;

		_ui_context.symbols = make_shared<symbol_resolver>([this, alive] (unsigned int persistent_id) {
			if (*alive)
			{
				send(request_module_metadata, persistent_id);
				LOG(PREAMBLE "requested metadata from remote...") % A(this) % A(persistent_id);
			}
		});
		_ui_context.threads = make_shared<threads_model>([this] (const vector<unsigned int> &threads) {
			send(request_threads_info, threads);
		});
		_ui_context.patches = make_shared<image_patch_model>(_ui_context.symbols, [this] (unsigned int persistent_id, const vector<unsigned int> &rva, bool apply) {
			send(apply ? request_apply_patches : request_revert_patches, persistent_id, rva);
		});

		LOG(PREAMBLE "constructed...") % A(this);
	}

	frontend::~frontend()
	{
		*_alive = false;
		LOG(PREAMBLE "destroyed...") % A(this);
	}

	void frontend::disconnect_session() throw()
	{
		_outbound.disconnect();
		LOG(PREAMBLE "disconnect requested locally...") % A(this);
	}

	void frontend::disconnect() throw()
	{	LOG(PREAMBLE "disconnected by remote...") % A(this);	}

	void frontend::message(const_byte_range payload)
	{
		unsigned persistent_id;
		buffer_reader reader(payload);
		strmd::deserializer<buffer_reader, packer> archive(reader);
		initialization_data idata;
		loaded_modules lmodules;
		module_info_metadata mmetadata;
		messages_id c;
		unsigned int token;

		switch (archive(c), c)
		{
		case init:
			archive(idata);
			_ui_context.executable = idata.executable;
			_ui_context.model = functions_list::create(idata.ticks_per_second, _ui_context.symbols, _ui_context.threads);
			initialized(_ui_context);
			schedule_update_request();
			LOG(PREAMBLE "initialized...") % A(this) % A(idata.executable) % A(idata.ticks_per_second);
			return;
		}

		switch (archive(token), c)
		{
		case response_modules_loaded:
			archive(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				_ui_context.symbols->add_mapping(*i);
			break;

		case legacy_update_statistics:
			LOG(PREAMBLE "non-threaded legacy_update_statistics is no longer supported. Are you using older version of the collector library?");
			break;

		case response_statistics_update:
			if (_ui_context.model)
				archive(*_ui_context.model, _serialization_context);
			_ui_context.threads->notify_threads(_serialization_context.threads.begin(), _serialization_context.threads.end());
			schedule_update_request();
			break;

		case response_module_metadata:
			archive(persistent_id);
			archive(mmetadata);
			LOG(PREAMBLE "received metadata...") % A(this) % A(persistent_id) % A(mmetadata.symbols.size()) % A(mmetadata.source_files.size());
			_ui_context.symbols->add_metadata(persistent_id, mmetadata);
			break;

		case response_threads_info:
			archive(*_ui_context.threads);
			break;

		default:
			break;
		}
	}

	void frontend::schedule_update_request()
	{	_queue.schedule([this] {	send(request_update, 0);	}, c_updateInterval);	}

	template <typename DataT>
	void frontend::send(messages_id command, const DataT &data)
	{
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);
		unsigned token = 0u;

		archive(command);
		archive(token);
		archive(data);
		_outbound.message(const_byte_range(_buffer.data(), _buffer.size()));
	}

	template <typename Data1T, typename Data2T>
	void frontend::send(messages_id command, const Data1T &data1, const Data2T &data2)
	{
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);
		unsigned token = 0u;

		archive(command);
		archive(token);
		archive(data1);
		archive(data2);
		_outbound.message(const_byte_range(_buffer.data(), _buffer.size()));
	}
}
