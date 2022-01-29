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
#include "serialization.h"

#include <common/argument_traits.h>
#include <common/noncopyable.h>
#include <functional>
#include <scheduler/private_queue.h>
#include <unordered_map>

namespace micro_profiler
{
	namespace ipc
	{
		class server_session : public channel, noncopyable
		{
		public:
			class response;
			typedef unsigned long long token_t;

		public:
			server_session(channel &outbound, scheduler::queue *apartment_queue = nullptr);

			void set_disconnect_handler(const std::function<void ()> &handler);

			template <typename F>
			void add_handler(int request_id, const F &handler);

			template <typename FormatterT>
			void message(int message_id, const FormatterT &message_formatter);

		private:
			template <typename ArgumentsT, typename U>
			struct handler_thunk;
			typedef std::function<void (response &response_, deserializer &payload)> handler_t;

		private:
			// channel methods
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

			void schedule_continuation(token_t token, const std::function<void (response &response_)> &continuation_handler);

		private:
			channel &_outbound;
			pod_vector<byte> _outbound_buffer;
			std::function<void ()> _disconnect_handler;
			std::unordered_map<int /*request_id*/, handler_t> _handlers;
			std::unique_ptr<scheduler::private_queue> _apartment_queue;
		};

		template <typename U>
		struct server_session::handler_thunk<arguments_pack<void (server_session::response &response_)>, U>
		{
			void operator ()(response &response_, deserializer &) const
			{	underlying(response_);	}

			U underlying;
		};

		template <typename T1, typename U>
		struct server_session::handler_thunk<arguments_pack<void (server_session::response &response_, T1 arg1)>, U>
		{
			void operator ()(response &response_, deserializer &payload) const
			{
				payload(arg1);
				underlying(response_, arg1);
			}

			U underlying;
			mutable typename std::remove_const<typename std::remove_reference<T1>::type>::type arg1;
		};

		class server_session::response : noncopyable
		{
		public:
			void operator ()(int response_id);
			template <typename ResultT>
			void operator ()(int response_id, const ResultT &result);

			template <typename FormatterT>
			void respond(int response_id, const FormatterT &response_formatter);

			void defer(const std::function<void (response &response_)> &continuation_handler);

		private:
			response(server_session &owner, token_t token, bool deferral_enabled);

			void operator &();
			void operator &() const;

		private:
			std::function<void (response &response_)> continuation;

		private:
			server_session &_owner;
			const token_t _token;
			const bool _deferral_enabled;

		private:
			friend class server_session;
		};



		template <typename F>
		inline void server_session::add_handler(int request_id, const F &handler)
		{
			handler_thunk<typename argument_traits<F>::types, F> thunk = {	handler,	};
			_handlers[request_id] = thunk;
		}

		template <typename FormatterT>
		inline void server_session::message(int message_id, const FormatterT &message_formatter)
		{
			{
				buffer_writer< pod_vector<byte> > bw(_outbound_buffer);
				serializer ser(bw);

				ser(message_id);
				message_formatter(ser);
			}
			_outbound.message(const_byte_range(_outbound_buffer.data(), _outbound_buffer.size()));
		}


		inline void server_session::response::operator ()(int response_id)
		{	respond(response_id, [] (serializer &) {	});	}

		template <typename ResultT>
		inline void server_session::response::operator ()(int response_id, const ResultT &result)
		{	respond(response_id, [&result] (serializer &ser) {	ser(result);	});	}

		template <typename FormatterT>
		inline void server_session::response::respond(int response_id, const FormatterT &response_formatter)
		{
			_owner.message(response_id, [&] (serializer &ser) {
				ser(_token);
				response_formatter(ser);
			});
		}
	}
}
