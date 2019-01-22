//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
			const char c_localhost[] = "127.0.0.1";

			struct sockets_initializer : noncopyable
			{
				sockets_initializer();
				~sockets_initializer();
			};

			struct host_port
			{
				host_port(const char *address, const char *default_host = c_localhost);

				std::string host;
				unsigned short port;
			};

			class socket_handle : noncopyable
			{
			public:
				template <typename T>
				explicit socket_handle(T s);
				socket_handle(socket_handle &other);
				~socket_handle();

				void reset() throw();
				void reset(int s);
				operator int() const;

			private:
				int _socket;
			};



			inline host_port::host_port(const char *address, const char *default_host)
			{
				if (const char *delim = strchr(address, ':'))
					host.assign(address, delim), port = static_cast<unsigned short>(atoi(delim + 1));
				else
					host = default_host, port = static_cast<unsigned short>(atoi(address));
			}


			template <typename T>
			inline socket_handle::socket_handle(T s)
				: _socket(0)
			{	reset(static_cast<int>(s));	}

			inline socket_handle::socket_handle(socket_handle &other)
				: _socket(other._socket)
			{	other._socket = 0;	}

			inline socket_handle::~socket_handle()
			{	reset();	}

			inline void socket_handle::reset() throw()
			{
				if (_socket)
					::close(_socket);
				_socket = 0;
			}

			inline void socket_handle::reset(int s)
			{
				if (s == -1)
					throw std::invalid_argument("invalid socket");
				reset();
				_socket = s;
			}

			inline socket_handle::operator int() const
			{	return _socket;	}
		}
	}
}
