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

#include <collector/frontend_controller.h>

#include <collector/entry.h>
#include <collector/image_patch.h>
#include <collector/statistics_bridge.h>
#include <common/time.h>
#include <common/module.h>
#include <set>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		class task
		{
		public:
			task(const function<void()> &f, timestamp_t period)
				: _task(f), _period(period), _expires_at(period + clock())
			{	}

			timestamp_t execute(timestamp_t &t)
			{
				if (t >= _expires_at)
				{
					_task();
					t = clock();
					_expires_at = t + _period;
				}
				return _expires_at;
			}

		private:
			function<void()> _task;
			timestamp_t _period;
			timestamp_t _expires_at;
		};
	}

	class frontend_controller::profiler_instance : public handle, noncopyable
	{
	public:
		profiler_instance(void *in_image_address, shared_ptr<image_load_queue> lqueue,
			shared_ptr<ref_counter_t> worker_refcount, shared_ptr<mt::event> exit_event);
		virtual ~profiler_instance() throw();

	private:
		const void *_in_image_address;
		shared_ptr<image_load_queue> _image_load_queue;
		shared_ptr<ref_counter_t> _worker_refcount;
		shared_ptr<mt::event> _exit_event;
		std::auto_ptr<image_patch> _patch;
	};


	extern "C" micro_profiler::calls_collector *g_collector_ptr;

	frontend_controller::profiler_instance::profiler_instance(void *in_image_address,
			shared_ptr<image_load_queue> lqueue, shared_ptr<ref_counter_t> worker_refcount,
			shared_ptr<mt::event> exit_event)
		: _in_image_address(in_image_address), _image_load_queue(lqueue), _worker_refcount(worker_refcount),
			_exit_event(exit_event)
	{
		//set<const void *> patched;
		//module_info mi = get_module_info(in_image_address);
		//shared_ptr<image_info> ii(new offset_image_info(image_info::load(mi.path.c_str()), (size_t)mi.load_address));
		//_patch.reset(new image_patch(ii, g_collector_ptr));

		//_patch->apply_for([&] (const symbol_info &symbol) -> bool {
		//	if (symbol.name == "_VEC_memcpy")
		//		return false;
		//	return patched.insert(symbol.body.begin()).second;
		//});


		_image_load_queue->load(in_image_address);
	}

	frontend_controller::profiler_instance::~profiler_instance() throw()
	{
		_image_load_queue->unload(_in_image_address);
		if (1 == _worker_refcount->fetch_add(-1))
			_exit_event->set();
	}


	frontend_controller::frontend_controller(calls_collector_i &collector, const frontend_factory_t& factory)
		: _collector(collector), _factory(factory), _image_load_queue(new image_load_queue),
			_worker_refcount(new ref_counter_t(0))
	{	}

	frontend_controller::~frontend_controller()
	{
		if (_frontend_thread.get())
			_frontend_thread->detach();
	}

	handle *frontend_controller::profile(void *in_image_address)
	{
		if (!_worker_refcount->fetch_add(1))
		{
			shared_ptr<mt::event> exit_event(new mt::event);
			auto_ptr<mt::thread> frontend_thread(new mt::thread(bind(&frontend_controller::frontend_worker,
				_frontend_thread.get(), _factory, &_collector, _image_load_queue, exit_event)));

			_frontend_thread.release();

			swap(_exit_event, exit_event);
			swap(_frontend_thread, frontend_thread);
		}
		return new profiler_instance(in_image_address, _image_load_queue, _worker_refcount, _exit_event);
	}

	void frontend_controller::force_stop()
	{
		if (_exit_event && _frontend_thread.get())
		{
			_exit_event->set();
			_frontend_thread->join();
			_frontend_thread.reset();
		}
	}

	void frontend_controller::frontend_worker(mt::thread *previous_thread, const frontend_factory_t &factory,
		calls_collector_i *collector, const shared_ptr<image_load_queue> &lqueue,
		const shared_ptr<mt::event> &exit_event)
	{
		if (previous_thread)
			previous_thread->join();
		delete previous_thread;

//		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

		statistics_bridge b(*collector, factory, lqueue);
		timestamp_t t = clock();

		task tasks[] = {
			task(bind(&statistics_bridge::analyze, &b), 10),
			task(bind(&statistics_bridge::update_frontend, &b), 67),
		};

		for (mt::milliseconds p(0); !exit_event->wait(p); )
		{
			timestamp_t expires_at = numeric_limits<timestamp_t>::max();

			t = clock();
			for (task *i = tasks; i != tasks + sizeof(tasks) / sizeof(task); ++i)
			{
				timestamp_t next_at = i->execute(t);

				if (next_at < expires_at)
					expires_at = next_at;
			}
			p = mt::milliseconds(expires_at > t ? expires_at - t : 0);
		}

		b.analyze();
		b.update_frontend();
	}
}
