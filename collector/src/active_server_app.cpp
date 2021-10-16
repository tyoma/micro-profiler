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

#include <collector/active_server_app.h>

#include <common/time.h>
#include <ipc/marshalled_session.h>
#include <ipc/server_session.h>
#include <logger/log.h>
#include <scheduler/scheduler.h>
#include <scheduler/task_queue.h>

#define PREAMBLE "Active server: "

using namespace std;

namespace micro_profiler
{
	using namespace ipc;

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

	active_server_app::active_server_app()
		: _session(nullptr), _exit_requested(false), _exit_confirmed(false),
			_queue(new scheduler::task_queue([] {	return mt::milliseconds(clock());	}))
	{	}

	active_server_app::~active_server_app()
	{	stop();	}

	void active_server_app::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	_queue->schedule(move(task), defer_by);	}

	void active_server_app::start(const frontend_factory_t &factory)
	{
		LOG(PREAMBLE "starting worker thread.");
		_frontend_thread.reset(new mt::thread([this, factory] {	worker(factory);	}));
	}

	void active_server_app::stop()
	{
		if (_frontend_thread.get())
		{
			LOG(PREAMBLE "stop request received...") % A(this);
			_queue->schedule([this] {
				(_session && finalize_session(*_session) ? _exit_requested : _exit_confirmed) = true;
				LOG(PREAMBLE "processing stop request...") % A(_exit_confirmed);
			});
			_frontend_thread->join();
			_frontend_thread.reset();
			LOG(PREAMBLE "worker thread exited.");
		}
	}

	void active_server_app::worker(const frontend_factory_t &factory)
	{
		const auto qw = make_shared<queue_wrapper>(*_queue);
		unique_ptr<marshalled_active_session> s;
		const auto frontend_disconnected = [&] {
			auto exit_confirmed = false;

			if (_exit_requested)
				exit_confirmed = _exit_confirmed = true;
			_session = nullptr;
			s.reset();

			LOG(PREAMBLE "remote session disconnected...") % A(exit_confirmed);
		};

		s.reset(new marshalled_active_session(factory, qw, [&] (channel &outbound) -> channel_ptr_t {
			const auto session = make_shared<server_session>(outbound);

			initialize_session(*session);
			session->set_disconnect_handler(frontend_disconnected);
			_session = session.get();
			return session;
		}));

		LOG(PREAMBLE "worker started!");
		do
		{
			_queue->wait();
			_queue->execute_ready(mt::milliseconds(100));
		} while (!_exit_confirmed);
		LOG(PREAMBLE "worker thread exiting...");
	}

	bool active_server_app::finalize_session(ipc::server_session &/*session*/)
	{	return false;	}
}
