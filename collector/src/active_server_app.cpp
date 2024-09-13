//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <tasker/scheduler.h>

#define PREAMBLE "Active server: "

using namespace std;

namespace micro_profiler
{
	using namespace ipc;

	active_server_app::active_server_app(events &events_)
		: _events(events_), _session(nullptr), _exit_requested(false), _exit_confirmed(false),
			_queue([] {	return mt::milliseconds(micro_profiler::clock());	}), _thread([this] {	worker();	})
	{
		LOG(PREAMBLE "constructed...") % A(this);
	}

	active_server_app::~active_server_app()
	{
		LOG(PREAMBLE "destroying...") % A(this);
		schedule([this] {
			LOG(PREAMBLE "finalizing...") % A(this) % A(_session);
			(_session && _events.finalize_session(*_session) ? _exit_requested : _exit_confirmed) = true;
			LOG(PREAMBLE "processing stop request...") % A(this) % A(_exit_confirmed);
		});
		_thread.join();
		LOG(PREAMBLE "destroyed...") % A(this);
	}

	void active_server_app::connect(const client_factory_t &factory)
	{
		const auto disconnected = [this] {
			auto exit_confirmed = false;

			if (_exit_requested)
				exit_confirmed = _exit_confirmed = true;
			_session = nullptr;
			_active_session.reset();

			LOG(PREAMBLE "remote session disconnected...") % A(exit_confirmed);
		};
		const auto server_session_factory = [this, disconnected] (channel &outbound) -> channel_ptr_t {
			const auto session = make_shared<server_session>(outbound);

			_events.initialize_session(*session);
			session->set_disconnect_handler(disconnected);
			_session = session.get();
			return session;
		};

		LOG(PREAMBLE "connection scheduled...");
		schedule([this, factory, server_session_factory] {
			_active_session.reset(new marshalled_active_session(factory, *this, server_session_factory));
			LOG(PREAMBLE "connection created!");
		});
	}

	void active_server_app::schedule(function<void ()> &&task, mt::milliseconds defer_by)
	{	_queue.schedule(move(task), defer_by);	}

	void active_server_app::worker()
	{
		LOG(PREAMBLE "worker started!");
		do
		{
			_queue.wait();
			_queue.execute_ready(mt::milliseconds(100));
		} while (!_exit_confirmed);
		LOG(PREAMBLE "destroying the active session...");
		_active_session.reset();
		LOG(PREAMBLE "worker thread exiting...");
	}
}
