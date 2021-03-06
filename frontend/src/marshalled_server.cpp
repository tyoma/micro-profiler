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

#include <frontend/marshalled_server.h>

#include <functional>
#include <logger/log.h>
#include <mt/mutex.h>
#include <scheduler/scheduler.h>
#include <vector>

#define PREAMBLE "Marshalled Server: "

using namespace std;

namespace micro_profiler
{
	namespace
	{
		template <typename PtrT>
		void schedule_destroy(scheduler::queue &queue, PtrT &ptr)
		{
			function<void ()> destroy = [ptr] {	};

			ptr = PtrT();
			queue.schedule(move(destroy));
		}
	}


	class marshalled_server::lifetime
	{
	public:
		lifetime()
			: _alive(true)
		{	}

		void mark_destroyed()
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			_alive = false;
		}

		template <typename F>
		void execute_safe(const F &f)
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			if (_alive)
				f();
		}

		template <typename T>
		static void schedule_safe(const shared_ptr<lifetime> &self, scheduler::queue &queue, const T &task)
		{	queue.schedule([self, task] {	self->execute_safe(task);	});	}

	private:
		mt::mutex _mtx;
		bool _alive;
	};

	marshalled_server::outbound_wrapper::outbound_wrapper(ipc::channel &underlying)
		: _lifetime(make_shared<lifetime>()), _underlying(underlying)
	{	}

	void marshalled_server::outbound_wrapper::stop()
	{	_lifetime->mark_destroyed();	}

	void marshalled_server::outbound_wrapper::disconnect() throw()
	{	_lifetime->execute_safe([this] {	_underlying.disconnect();	});	}

	void marshalled_server::outbound_wrapper::message(const_byte_range payload)
	{	_lifetime->execute_safe([this, payload] {	_underlying.message(payload);	});	}


	marshalled_server::marshalled_session::marshalled_session(shared_ptr<scheduler::queue> queue, ipc::channel &outbound)
		: _lifetime(make_shared<lifetime>()), _queue(queue), _outbound(make_shared<outbound_wrapper>(outbound))
	{	}

	marshalled_server::marshalled_session::~marshalled_session()
	{
		LOG(PREAMBLE "destroying marshalled session and scheduling an underlying session destruction...")
			% A(this) % A(_underlying.get());
		_lifetime->mark_destroyed();
		_outbound->stop();
		schedule_destroy(*_queue, _underlying);
	}

	void marshalled_server::marshalled_session::create_underlying(shared_ptr<ipc::server> underlying_server)
	{
		LOG(PREAMBLE "scheduling an underlying session creation...") % A(this) % A(underlying_server.get());
		lifetime::schedule_safe(_lifetime, *_queue, [this, underlying_server] {
			typedef pair< shared_ptr<ipc::channel>, shared_ptr<ipc::channel> > composite_t;

			const auto composite = make_shared<composite_t>(_outbound, underlying_server->create_session(*_outbound));

			_underlying = shared_ptr<ipc::channel>(composite, composite->second.get());
		});
	}

	void marshalled_server::marshalled_session::disconnect() throw()
	{	lifetime::schedule_safe(_lifetime, *_queue, [this]	{	_underlying->disconnect();	});	}

	void marshalled_server::marshalled_session::message(const_byte_range payload)
	{
		const auto data = make_shared< vector<byte> >(payload.begin(), payload.end());

		lifetime::schedule_safe(_lifetime, *_queue, [this, data]	{
			_underlying->message(const_byte_range(data->data(), data->size()));
		});
	}


	marshalled_server::marshalled_server(shared_ptr<ipc::server> underlying, shared_ptr<scheduler::queue> queue)
		: _lifetime(make_shared<lifetime>()), _underlying(underlying), _queue(queue)
	{	}

	marshalled_server::~marshalled_server()
	{
		if (_underlying)
			LOGE(PREAMBLE "underlying server was not explicitly reset!") % A(this) % A(_underlying.get());
	}

	void marshalled_server::stop()
	{
		_lifetime->mark_destroyed();
		_underlying = nullptr;
	}

	shared_ptr<ipc::channel> marshalled_server::create_session(ipc::channel &outbound)
	{
		shared_ptr<marshalled_session> msession;

		_lifetime->execute_safe([&] {
			msession = make_shared<marshalled_session>(_queue, outbound);
			msession->create_underlying(_underlying);
		});
		return msession;
	}
}
