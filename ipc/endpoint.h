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

#include <common/range.h>
#include <memory>
#include <stdexcept>

namespace micro_profiler
{
	namespace ipc
	{
		struct initialization_failed : std::runtime_error
		{
			initialization_failed(const char *message);
		};

		struct protocol_not_supported : std::invalid_argument
		{
			protocol_not_supported(const char *message);
		};

		struct connection_refused : std::runtime_error
		{
			connection_refused(const char *message);
		};


		struct channel
		{
			virtual void disconnect() throw() = 0;
			virtual void message(const_byte_range payload) = 0;
		};

		struct server
		{
			virtual std::shared_ptr<channel> create_session(channel &outbound) = 0;
		};



		std::shared_ptr<channel> connect_client(const char *typed_destination_endpoint_id, channel &inbound);
		std::shared_ptr<void> run_server(const char *typed_endpoint_id, const std::shared_ptr<server> &factory);


		inline initialization_failed::initialization_failed(const char *message)
			: std::runtime_error(message)
		{	}


		inline protocol_not_supported::protocol_not_supported(const char *message)
			: std::invalid_argument(message)
		{	}


		inline connection_refused::connection_refused(const char *message)
			: std::runtime_error(message)
		{	}
	}
}
