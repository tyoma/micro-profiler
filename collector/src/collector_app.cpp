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

#include <collector/collector_app.h>

#include <collector/module_tracker.h>
#include <collector/statistics_bridge.h>
#include <collector/serialization.h>
#include <collector/thread_monitor.h>

#include <common/time.h>
#include <logger/log.h>
#include <strmd/deserializer.h>

#define PREAMBLE "Collector app: "

using namespace std;

namespace micro_profiler
{
	collector_app::collector_app(const frontend_factory_t &factory, const shared_ptr<calls_collector> &collector,
			const overhead &overhead_, const shared_ptr<thread_monitor> &thread_monitor_)
		: _queue([] {	return mt::milliseconds(clock());	}), _collector(collector), _module_tracker(new module_tracker),
			_thread_monitor(thread_monitor_), _exit(false)
	{
		_frontend_thread.reset(new mt::thread([this, factory, overhead_] {
			worker(factory, overhead_);
		}));
	}

	collector_app::~collector_app()
	{	stop();	}

	void collector_app::stop()
	{
		if (_frontend_thread.get())
		{
			_collector->flush();
			_queue.schedule([this] {	this->_exit = true;	});
			_frontend_thread->join();
			_frontend_thread.reset();
		}
	}

	void collector_app::disconnect() throw()
	{	}

	void collector_app::message(const_byte_range payload)
	try
	{
		buffer_reader reader(payload);
		strmd::deserializer<buffer_reader, packer> d(reader);
		commands c;
		unsigned int persistent_id;
		vector<thread_monitor::thread_id> thread_ids;

		switch (d(c), c)
		{
		case request_metadata:
			d(persistent_id);
			_queue.schedule([this, persistent_id] {	_bridge->send_module_metadata(persistent_id);	});
			break;

		case request_threads_info:
			d(thread_ids);
			_queue.schedule([this, thread_ids] {	_bridge->send_thread_info(thread_ids);	});
			break;

		default:
			break;
		}
	}
	catch (const exception &/*e*/)
	{
//		LOG(PREAMBLE "caught an exception while processing frontend request...") % A(e.what());
	}

	void collector_app::worker(const frontend_factory_t &factory, const overhead &overhead_)
	{
		shared_ptr<ipc::channel> frontend = factory(*this);
		_bridge.reset(new statistics_bridge(*_collector, overhead_, *frontend, _module_tracker, _thread_monitor));
		function<void ()> analyze;
		const auto analyze_ = [&] {
			_bridge->analyze();
			_queue.schedule(function<void ()>(analyze), mt::milliseconds(10));
		};
		function<void ()> update_frontend;
		const auto update_frontend_ = [&] {
			_bridge->update_frontend();
			_queue.schedule(function<void ()>(update_frontend), mt::milliseconds(40));
		};

		_queue.schedule(function<void ()>(analyze = analyze_), mt::milliseconds(10));
		_queue.schedule(function<void ()>(update_frontend = update_frontend_), mt::milliseconds(40));
		while (!_exit)
		{
			_queue.wait();
			_queue.execute_ready(mt::milliseconds(100));
		}
		_bridge->analyze();
		_bridge->update_frontend();
	}
}
