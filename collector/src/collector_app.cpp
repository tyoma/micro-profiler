//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/calibration.h>
#include <collector/module_tracker.h>
#include <collector/statistics_bridge.h>
#include <collector/serialization.h>

#include <common/time.h>
#include <logger/log.h>
#include <strmd/deserializer.h>

#define PREAMBLE "Collector app: "

using namespace std;

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

	collector_app::collector_app(const frontend_factory_t &factory, const shared_ptr<calls_collector> &collector,
			const overhead &overhead_)
		: _collector(collector), _module_tracker(new module_tracker)
	{	_frontend_thread.reset(new mt::thread(bind(&collector_app::worker, this, factory, overhead_)));	}

	collector_app::~collector_app()
	{	stop();	}

	void collector_app::stop()
	{
		if (_frontend_thread.get())
		{
			_exit.set();
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

		switch (d(c), c)
		{
		case request_metadata:
			d(persistent_id);
			_bridge->send_module_metadata(persistent_id);
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
		timestamp_t t = clock();

		_bridge.reset(new statistics_bridge(*_collector, overhead_, *frontend, _module_tracker));

		task tasks[] = {
			task(bind(&statistics_bridge::analyze, _bridge.get()), 10),
			task(bind(&statistics_bridge::update_frontend, _bridge.get()), 67),
		};

		for (mt::milliseconds p(0); !_exit.wait(p); )
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

		_bridge->analyze();
		_bridge->update_frontend();
	}
}
