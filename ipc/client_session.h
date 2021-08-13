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

#include <common/pod_vector.h>
#include <common/serialization.h>
#include <map>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>

namespace micro_profiler
{
	namespace ipc
	{
		class client_session : channel
		{
		public:
			typedef strmd::deserializer<buffer_reader, packer> deserializer;
			typedef std::function<void (deserializer &payload_deserializer)> callback_t;

		public:
			template <typename ChannelFactoryT>
			client_session(const ChannelFactoryT &connection_factory);

			template <typename RequestT, typename ResponseCallbackT>
			void request(std::shared_ptr<void> &handle, int id, const RequestT &payload, int response_id,
				const ResponseCallbackT &callback);

			template <typename RequestT, typename MultiResponseCallbackT>
			void request(std::shared_ptr<void> &handle, int id, const RequestT &payload,
				const MultiResponseCallbackT &callback);

		private:
			typedef unsigned long long token_t;
			typedef std::map<std::pair<int, token_t>, callback_t> callbacks_t;
			typedef strmd::serializer<buffer_writer< pod_vector<byte> >, packer> serializer;

		private:
			virtual void disconnect() throw() override;
			virtual void message(const_byte_range payload) override;

			template <typename RequestT, typename CallbackConstructorT>
			void request_internal(int id, const RequestT &payload,
				const CallbackConstructorT &callback_ctor);

		private:
			pod_vector<byte> _buffer;
			token_t _token;
			std::shared_ptr<channel> _outbound;
			std::shared_ptr<callbacks_t> _callbacks;
		};



		template <typename ChannelFactoryT>
		inline client_session::client_session(const ChannelFactoryT &connection_factory)
			: _token(1), _callbacks(make_shared<callbacks_t>())
		{	_outbound = connection_factory(*this);	}

		template <typename RequestT, typename ResponseCallbackT>
		inline void client_session::request(std::shared_ptr<void> &handle, int id, const RequestT &payload,
			int response_id, const ResponseCallbackT &callback)
		{
			request_internal(id, payload, [&] (token_t token) {
				auto callbacks = _callbacks;
				auto i = callbacks->insert(std::make_pair(std::make_pair(response_id, token), callback)).first;

				handle.reset(&*i, [callbacks, i] (...) {	callbacks->erase(i);	});
			});
		}

		template <typename RequestT, typename MultiResponseCallbackT>
		inline void client_session::request(std::shared_ptr<void> &handle, int id, const RequestT &payload,
			const MultiResponseCallbackT &callback)
		{
			request_internal(id, payload, [&] (token_t token) {
				auto callbacks = _callbacks;

				for (auto i = std::begin(callback); i != std::end(callback); ++i)
				{
					auto j = callbacks->insert(std::make_pair(std::make_pair(i->first, token), i->second)).first;

					if (handle)
					{
						auto h2 = handle;

						handle = shared_ptr<void>(&*j, [h2, callbacks, j] (...) {	callbacks->erase(j);	});
					}
					else
					{
						handle.reset(&*j, [callbacks, j] (...) {	callbacks->erase(j);	});
					}
				}
			});
		}

		template <typename RequestT, typename CallbackConstructorT>
		inline void client_session::request_internal(int id, const RequestT &payload,
			const CallbackConstructorT &callback_ctor)
		{
			_buffer.clear();
			buffer_writer< pod_vector<byte> > w(_buffer);
			serializer s(w);
			auto token = _token++;

			s(id);
			s(token);
			s(payload);
			_outbound->message(const_byte_range(_buffer.data(), _buffer.size()));
			callback_ctor(token);
		}

	}
}
