//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "endpoint.h"

#include <common/noncopyable.h>
#include <scheduler/private_queue.h>

namespace scheduler
{
	struct queue;
}

namespace micro_profiler
{
	namespace ipc
	{
		class lifetime;

		class marshalled_active_session : channel, noncopyable
		{
		public:
			template <typename ConnectionFactoryT, typename ServerSessionFactoryT>
			marshalled_active_session(const ConnectionFactoryT &connection_factory,
				scheduler::queue &apartment_queue, const ServerSessionFactoryT &server_session_factory);

		private:
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

		private:
			channel_ptr_t _connection_outbound, _server_inbound;
			scheduler::private_queue _apartment_queue;
		};


		class marshalled_passive_session : public channel, noncopyable
		{
		public:
			marshalled_passive_session(scheduler::queue &apartment_queue, channel &outbound);
			~marshalled_passive_session();

			void create_underlying(std::shared_ptr<server> underlying_server);

		private:
			class outbound_wrapper;

		private:
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

		private:
			const std::shared_ptr<lifetime> _lifetime;
			scheduler::queue &_apartment_queue;
			channel_ptr_t _underlying;
			std::shared_ptr<outbound_wrapper> _outbound;
		};

		class marshalled_passive_session::outbound_wrapper : public channel, noncopyable
		{
		public:
			outbound_wrapper(channel &underlying);

			void stop();

		private:
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

		private:
			const std::shared_ptr<lifetime> _lifetime;
			channel &_underlying;
		};



		template <typename ConnectionFactoryT, typename ServerSessionFactoryT>
		inline marshalled_active_session::marshalled_active_session(const ConnectionFactoryT &connection_factory,
				scheduler::queue &apartment_queue, const ServerSessionFactoryT &server_session_factory)
			: _apartment_queue(apartment_queue)
		{
			_connection_outbound = connection_factory(*this);
			_server_inbound = server_session_factory(*_connection_outbound);
		}

	}
}
