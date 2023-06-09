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

#include <ipc/marshalled_session.h>

#include "lifetime.h"

#include <functional>
#include <logger/log.h>
#include <mt/mutex.h>
#include <scheduler/scheduler.h>
#include <vector>

#define PREAMBLE "Marshalled Session: "

using namespace std;

namespace micro_profiler
{
	namespace ipc
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


		void marshalled_active_session::disconnect() throw()
		{
			_apartment_queue.schedule([this] {
				_server_inbound->disconnect();
			});
		}

		void marshalled_active_session::message(const_byte_range payload)
		{
			const auto data = make_shared< vector<byte> >(payload.begin(), payload.end());

			_apartment_queue.schedule([this, data]	{
				_server_inbound->message(const_byte_range(data->data(), data->size()));
			});
		}


		marshalled_passive_session::marshalled_passive_session(scheduler::queue &apartment_queue, ipc::channel &outbound)
			: _lifetime(make_shared<lifetime>()), _apartment_queue(apartment_queue),
				_outbound(make_shared<outbound_wrapper>(outbound))
		{	}

		marshalled_passive_session::~marshalled_passive_session()
		{
			LOG(PREAMBLE "destroying marshalled session and scheduling an underlying session destruction...")
				% A(this) % A(_underlying.get());
			_lifetime->mark_destroyed();
			_outbound->stop();
			schedule_destroy(_apartment_queue, _underlying);
		}

		void marshalled_passive_session::create_underlying(shared_ptr<ipc::server> underlying_server)
		{
			LOG(PREAMBLE "scheduling an underlying session creation...") % A(this) % A(underlying_server.get());
			lifetime::schedule_safe(_lifetime, _apartment_queue, [this, underlying_server] {
				typedef pair<channel_ptr_t, channel_ptr_t> composite_t;

				const auto composite = make_shared<composite_t>(_outbound, underlying_server->create_session(*_outbound));

				_underlying = channel_ptr_t(composite, composite->second.get());
			});
		}

		void marshalled_passive_session::disconnect() throw()
		{	lifetime::schedule_safe(_lifetime, _apartment_queue, [this]	{	_underlying->disconnect();	});	}

		void marshalled_passive_session::message(const_byte_range payload)
		{
			const auto data = make_shared< vector<byte> >(payload.begin(), payload.end());

			lifetime::schedule_safe(_lifetime, _apartment_queue, [this, data]	{
				_underlying->message(const_byte_range(data->data(), data->size()));
			});
		}


		marshalled_passive_session::outbound_wrapper::outbound_wrapper(ipc::channel &underlying)
			: _lifetime(make_shared<lifetime>()), _underlying(underlying)
		{	}

		void marshalled_passive_session::outbound_wrapper::stop()
		{	_lifetime->mark_destroyed();	}

		void marshalled_passive_session::outbound_wrapper::disconnect() throw()
		{	_lifetime->execute_safe([this] {	_underlying.disconnect();	});	}

		void marshalled_passive_session::outbound_wrapper::message(const_byte_range payload)
		{	_lifetime->execute_safe([this, payload] {	_underlying.message(payload);	});	}
	}
}
