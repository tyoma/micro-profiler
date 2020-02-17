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

#include <common/noncopyable.h>
#include <ipc/endpoint.h>

namespace micro_profiler
{
	class marshalling_server;

	namespace ipc
	{
		class ipc_manager : noncopyable
		{
		public:
			typedef std::pair<unsigned short /*start*/, unsigned short /*size*/> port_range;

		public:
			ipc_manager(const std::shared_ptr<server> &underlying, port_range range_);
			~ipc_manager();

			unsigned short get_sockets_port() const;
			bool remote_sockets_enabled() const;
			void enable_remote_sockets(bool enable);

			bool com_enabled() const;
			void enable_com(bool enable);

			static std::string format_endpoint(const std::string &interface_, unsigned short port);

		private:
			static std::shared_ptr<void> probe_create_server(const std::shared_ptr<server> &underlying,
				const std::string &interface_, unsigned short &port, port_range range_);

		private:
			const std::shared_ptr<marshalling_server> _underlying;
			const port_range _range;

			std::shared_ptr<void> _sockets_server;
			bool _remote_enabled;
			unsigned short _port;

			std::shared_ptr<void> _com_server;
		};
	}
}
