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
#include <frontend/serialization.h>
#include <frontend/symbol_resolver.h>

#include <common/memory.h>
#include <logger/log.h>
#include <scheduler/scheduler.h>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>

#define PREAMBLE "Frontend: "

using namespace std;

namespace micro_profiler
{
	frontend::frontend(ipc::channel &outbound, shared_ptr<scheduler::queue> queue)
		: _outbound(outbound), _queue(queue), _alive(make_shared<bool>(true))
	{	LOG(PREAMBLE "constructed...") % A(this);	}

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

		switch (archive(c), c)
		{
		case init:
			archive(idata);
			_model = functions_list::create(idata.ticks_per_second, get_resolver(), get_threads());
			initialized(idata.executable, _model);
			update_and_schedule(_alive);
			LOG(PREAMBLE "initialized...") % A(this) % A(idata.executable) % A(idata.ticks_per_second);
			break;

		case response_modules_loaded:
			archive(lmodules);
			for (loaded_modules::const_iterator i = lmodules.begin(); i != lmodules.end(); ++i)
				get_resolver()->add_mapping(*i);
			break;

		case legacy_update_statistics:
			LOG(PREAMBLE "non-threaded legacy_update_statistics is no longer supported. Are you using older version of the collector library?");
			break;

		case response_statistics_update:
			if (_model)
				archive(*_model, _serialization_context);
			get_threads()->notify_threads(_serialization_context.threads.begin(), _serialization_context.threads.end());
			break;

		case response_module_metadata:
			archive(persistent_id);
			archive(mmetadata);
			LOG(PREAMBLE "received metadata...") % A(this) % A(persistent_id) % A(mmetadata.symbols.size()) % A(mmetadata.source_files.size());
			get_resolver()->add_metadata(persistent_id, mmetadata);
			break;

		case response_threads_info:
			archive(*_threads);
			break;

		default:
			break;
		}
	}

	template <typename DataT>
	void frontend::send(messages_id command, const DataT &data)
	{
		buffer_writer< pod_vector<byte> > writer(_buffer);
		strmd::serializer<buffer_writer< pod_vector<byte> >, packer> archive(writer);

		archive(command);
		archive(data);
		_outbound.message(const_byte_range(_buffer.data(), _buffer.size()));
	}

	void frontend::update_and_schedule(shared_ptr<bool> alive)
	{
		if (*alive)
		{
			send(request_update, 0 /*not being used yet*/);
			_queue->schedule([this, alive] {	update_and_schedule(alive);	}, mt::milliseconds(33));
		}
	}

	shared_ptr<symbol_resolver> frontend::get_resolver()
	{
		if (!_resolver)
		{
			weak_ptr<frontend> wself = shared_from_this();

			_resolver.reset(new symbol_resolver([wself] (unsigned int persistent_id) {
				if (shared_ptr<frontend> self = wself.lock())
				{
					self->send(request_module_metadata, persistent_id);
					LOG(PREAMBLE "requested metadata from remote...") % A(self.get()) % A(persistent_id);
				}
			}));
		}
		return _resolver;
	}

	shared_ptr<threads_model> frontend::get_threads()
	{
		if (!_threads)
		{
			_threads.reset(new threads_model([this] (const vector<unsigned int> &threads) {
				send(request_threads_info, threads);
			}));
		}
		return _threads;
	}
}
