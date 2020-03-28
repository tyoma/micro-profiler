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

#pragma once

#include "analyzer.h"
#include "thread_monitor.h"

#include <common/protocol.h>

namespace micro_profiler
{
	struct calls_collector_i;
	class module_tracker;
	struct overhead;

	namespace ipc
	{
		struct channel;
	}

	class statistics_bridge : noncopyable
	{
	public:
		statistics_bridge(calls_collector_i &collector, const overhead &overhead_, ipc::channel &frontend,
			const std::shared_ptr<module_tracker> &module_tracker_,
			const std::shared_ptr<thread_monitor> &thread_monitor_);

		void analyze();
		void update_frontend();
		void send_module_metadata(unsigned int persistent_id);
		void send_thread_info(const std::vector<thread_monitor::thread_id> &ids);

	private:
		template <typename DataT>
		void send(commands command, const DataT &data);

	public:
		pod_vector<byte> _buffer;
		std::vector< std::pair<thread_monitor::thread_id, thread_info> > _threads_buffer;
		analyzer _analyzer;
		calls_collector_i &_collector;
		ipc::channel &_frontend;
		const std::shared_ptr<module_tracker> _module_tracker;
		const std::shared_ptr<thread_monitor> _thread_monitor;
		mt::mutex _mutex;
	};
}
