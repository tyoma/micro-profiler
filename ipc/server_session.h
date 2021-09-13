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

#pragma once

#include "endpoint.h"

#include <common/noncopyable.h>
#include <common/pod_vector.h>
#include <common/serialization.h>
#include <common/stream.h>
#include <functional>
#include <scheduler/private_queue.h>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>

namespace micro_profiler
{
	namespace ipc
	{
		class server_session : public channel, noncopyable
		{
		public:
			class request;
			typedef strmd::serializer<buffer_writer< pod_vector<byte> >, packer> serializer;
			typedef unsigned long long token_t;

		public:
			server_session(channel &outbound, std::shared_ptr<scheduler::queue> queue = std::shared_ptr<scheduler::queue>());

			void set_disconnect_handler(const std::function<void () throw()> &handler);

			void add_handler(int request_id, const std::function<void (request &context)> &handler);

			template <typename PayloadT>
			void add_handler(int request_id,
				const std::function<void (request &context, const PayloadT &payload)> &handler);

			template <typename FormatterT>
			void message(int message_id, const FormatterT &message_formatter);

		private:
			typedef strmd::deserializer<buffer_reader, packer> deserializer;
			typedef std::function<void (request &context, deserializer &payload)> handler_t;

		private:
			// channel methods
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

			void schedule_continuation(token_t token, const std::function<void (request &context)> &continuation_handler);

		private:
			channel &_outbound;
			pod_vector<byte> _outbound_buffer;
			std::function<void () throw()> _disconnect_handler;
			std::unordered_map<int /*request_id*/, handler_t> _handlers;
			scheduler::private_queue _queue;
			const bool _deferral_enabled;
		};

		class server_session::request : noncopyable
		{
		public:
			template <typename FormatterT>
			void respond(int response_id, const FormatterT &response_formatter);

			void defer(const std::function<void (request &context)> &continuation_handler);

		private:
			request(server_session &owner, token_t token, bool deferral_enabled);

			void operator &();
			void operator &() const;

		private:
			std::function<void (request &context)> continuation;

		private:
			server_session &_owner;
			const token_t _token;
			const bool _deferral_enabled;

		private:
			friend class server_session;
		};



		template <typename PayloadT>
		inline void server_session::add_handler(int request_id,
			const std::function<void (request &context, const PayloadT &payload)> &handler)
		{
			auto payload_buffer = std::make_shared<PayloadT>();

			_handlers[request_id] = [handler, payload_buffer] (request &context, deserializer &payload_deserializer) {
				payload_deserializer(*payload_buffer);
				handler(context, *payload_buffer);
			};
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


		template <typename FormatterT>
		inline void server_session::request::respond(int response_id, const FormatterT &response_formatter)
		{
			_owner.message(response_id, [&] (serializer &ser) {
				ser(_token);
				response_formatter(ser);
			});
		}
	}
}
