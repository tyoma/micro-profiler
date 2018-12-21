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

#include <ipc/endpoint_sockets.h>

#include <arpa/inet.h>
#include <common/noncopyable.h>
#include <list>
#include <mt/thread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
#if defined(WIN32)
			struct winsock_initializer
			{
				winsock_initializer()
				{
					WSADATA data = { };
					::WSAStartup(MAKEWORD(2, 2), &data);
				}

				~winsock_initializer()
				{	::WSACleanup();	}
			};
#else
			struct winsock_initializer {	};
#endif

			class socket_h : noncopyable
			{
			public:
				explicit socket_h(int s)
					: _socket(s)
				{
					if (s == -1)
						throw runtime_error("invalid socket");
				}

				~socket_h()
				{	reset();	}

				void reset()
				{
					if (_socket)
					{
						::shutdown(_socket, 2);
						::close(_socket);
						_socket = 0;
					}
				}

				operator int() const
				{	return _socket;	}

			private:
				int _socket;
			};

			class session : /*outbound*/ ipc::channel
			{
			public:
				session(int s, ipc::server &factory);
				~session();

				void disconnect() throw();
				void message(const_byte_range payload);

			private:
				socket_h _socket;
				shared_ptr<ipc::channel> _inbound;
				auto_ptr<mt::thread> _thread;
			};


			class server
			{
			public:
				server(const char *endpoint_id, const shared_ptr<ipc::server> &factory);
				~server();

			private:
				winsock_initializer _initializer;
				socket_h _server_socket;
				auto_ptr<mt::thread> _server_thread;
				list< shared_ptr<session> > _sessions;
			};


			class endpoint : public ipc::endpoint
			{
				virtual shared_ptr<void> run_server(const char *endpoint_id, const shared_ptr<ipc::server> &factory);
			};



			session::session(int s, ipc::server &factory)
				: _socket(s)
			{
				_inbound = factory.create_session(*this);
				_thread.reset(new mt::thread([this] {
					sockets::byte_representation<unsigned int> size;
					vector<byte> buffer;

					for (int read; read = ::recv(_socket, size.bytes, sizeof(size.bytes), MSG_WAITALL), read > 0; )
					{
						size.reorder();
						buffer.resize(size.value);
						::recv(_socket, reinterpret_cast<char *>(&buffer[0]), size.value, MSG_WAITALL);
						_inbound->message(const_byte_range(&buffer[0], buffer.size()));
					}
					_inbound->disconnect();
					_inbound.reset();
				}));
			}

			session::~session()
			{
				_socket.reset();
				_thread->join();
			}

			void session::disconnect() throw()
			{	}

			void session::message(const_byte_range /*payload*/)
			{	}


			server::server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
				: _server_socket((int)::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
			{
				sockaddr_in service = {
					AF_INET,
					htons(static_cast<u_short>(atoi(endpoint_id))),
					inet_addr("127.0.0.1"),
					{ 0 }
				};

				if (::bind(_server_socket, (sockaddr *)&service, sizeof(service)))
					throw runtime_error("bind() failed");
				if (::listen(_server_socket, 5))
					throw runtime_error("listen() failed");
				_server_thread.reset(new mt::thread([this, factory] {
					for (int new_connection; new_connection = (int)::accept(_server_socket, NULL, NULL), new_connection != -1; )
						_sessions.push_back(shared_ptr<session>(new session(new_connection, *factory)));
				}));
			}

			server::~server()
			{
				_server_socket.reset();
				_server_thread->join();
			}


			shared_ptr<void> endpoint::run_server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
			{	return shared_ptr<void>(new server(endpoint_id, factory));	}

			shared_ptr<ipc::endpoint> create_endpoint()
			{	return shared_ptr<endpoint>(new endpoint);	}
		}
	}
}
