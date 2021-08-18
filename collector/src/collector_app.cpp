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

#include <collector/analyzer.h>
#include <collector/module_tracker.h>
#include <collector/serialization.h>
#include <collector/thread_monitor.h>

#include <common/time.h>
#include <ipc/marshalled_session.h>
#include <ipc/server_session.h>
#include <logger/log.h>
#include <scheduler/scheduler.h>

#define PREAMBLE "Collector app: "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		class queue_wrapper : public scheduler::queue
		{
		public:
			queue_wrapper(scheduler::task_queue &q)
				: _queue(q)
			{	}

			virtual void schedule(function<void ()> &&task, mt::milliseconds defer_by) override
			{	_queue.schedule(move(task), defer_by);	}

		private:
			scheduler::task_queue &_queue;
		};
	}

	collector_app::collector_app(const frontend_factory_t &factory, calls_collector_i &collector,
			const overhead &overhead_, thread_monitor &thread_monitor_, patch_manager &patch_manager_)
		: _queue([] {	return mt::milliseconds(clock());	}), _collector(collector), _thread_monitor(thread_monitor_),
			_patch_manager(patch_manager_), _module_tracker(new module_tracker), _exit(false)
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
			_collector.flush();
			_queue.schedule([this] {	this->_exit = true;	});
			_frontend_thread->join();
			_frontend_thread.reset();
		}
	}

	void collector_app::worker(const frontend_factory_t &factory, const overhead &overhead_)
	{
		shared_ptr<scheduler::queue> qw(new queue_wrapper(_queue));
		analyzer analyzer_(overhead_);
		shared_ptr<ipc::channel> inbound;
		ipc::marshalled_active_session s(factory, qw, [&] (ipc::channel &outbound) {
			return inbound = init_server(outbound, analyzer_);
		});
		function<void ()> analyze;
		const auto analyze_ = [&] {
			_collector.read_collected(analyzer_);
			_queue.schedule(function<void ()>(analyze), mt::milliseconds(10));
		};

		_queue.schedule(function<void ()>(analyze = analyze_), mt::milliseconds(10));
		while (!_exit)
		{
			_queue.wait();
			_queue.execute_ready(mt::milliseconds(100));
		}

		// A hack to retrieve last chunk of updates. Must be replaced with graceful disconnect logic.
		pod_vector<byte> data;
		buffer_writer< pod_vector<byte> > bw(data);
		ipc::server_session::serializer ser(bw);

		ser(request_update), ser(0u), ser(0u);
		_collector.read_collected(analyzer_);
		inbound->message(const_byte_range(data.data(), data.size()));
	}
}
