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
#include <functional>
#include <strmd/packer.h>
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

		public:
			server_session(channel &outbound);

			template <typename PayloadT>
			void add_handler(unsigned int request_id,
				const std::function<void (request &context, const PayloadT &payload)> &handler);

			template <typename FormatterT>
			void message(unsigned int message_id, const FormatterT &message_formatter);

		private:
			typedef strmd::deserializer<buffer_reader, packer> deserializer;
			typedef std::function<void (request &context, deserializer &payload)> handler_t;

		private:
			// channel methods
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

		private:
			std::unordered_map<unsigned int /*request_id*/, handler_t> _handlers;
		};

		class server_session::request : noncopyable
		{
		public:
			template <typename FormatterT>
			void respond(unsigned int response_id, const FormatterT &response_formatter);

		public:
			std::function<void (request &context)> continuation;

		private:
			request();

		private:
			friend class server_session;
		};



		template <typename PayloadT>
		inline void server_session::add_handler(unsigned int request_id,
			const std::function<void (request &context, const PayloadT &payload)> &handler)
		{
			auto payload_buffer = std::make_shared<PayloadT>();

			_handlers[request_id] = [handler, payload_buffer] (request &context, deserializer &payload_deserializer) {
				payload_deserializer(*payload_buffer);
				handler(context, *payload_buffer);
			};
		}
	}
}
