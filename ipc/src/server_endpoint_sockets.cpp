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

#include "server_endpoint_sockets.h"

#include <arpa/inet.h>
#include <common/noncopyable.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/types.h>

#pragma warning(disable: 4127)
#pragma warning(disable: 4389)

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
			namespace
			{
				void setup_socket(const socket_handle &s)
				{
					linger l = {};

					l.l_onoff = 1;
					if (::setsockopt(s, SOL_SOCKET, SO_LINGER, (const char*)&l, sizeof(l)) < 0)
						throw initialization_failed("setsockopt(..., SO_LINGER, ...) failed");
				};

				template <typename SocketT, typename T>
				int send_scalar(const SocketT &s, T value)
				{
					byte_representation<T> data;

					data.value = value;
					data.reorder();
					return ::send(s, data.bytes, sizeof(data.bytes), MSG_NOSIGNAL);
				}

				template <typename T>
				int recv_scalar(const socket_handle &s, T &value, int options)
				{
					byte_representation<T> data;
					int result = ::recv(s, data.bytes, sizeof(data.bytes), options);

					data.reorder();
					value = data.value;
					return result;
				}
			}

			socket_handler::socket_handler(unsigned id_, socket_handle &s, const handler_t &initial_handler)
				: id(id_), handler(initial_handler), _socket(s)
			{	}

			socket_handler::~socket_handler()
			{
				handler = handler_t(); // first - release the underlying session...
				_socket.reset(); // ... then - the socket
			}

			template <typename ContainerT>
			void socket_handler::run(ContainerT &handlers)
			{
				for (;;)
				{
					fd_set fds;

					FD_ZERO(&fds);
					for (typename ContainerT::const_iterator i = handlers.begin(); i != handlers.end(); ++i)
						FD_SET((*i)->_socket, &fds);					
					if (::select(FD_SETSIZE, &fds, NULL, NULL, NULL) < 0)
						return;
					for (typename ContainerT::iterator i = handlers.begin(); i != handlers.end(); )
					{
						switch(FD_ISSET((*i)->_socket, &fds) ? (*i)->handler(**i, (*i)->_socket) : proceed)
						{
						case proceed:
							++i;
							break;

						case remove_this:
							i = handlers.erase(i);
							break;

						case exit:
							return;
						}
					}
				}
			}


			session::session(unsigned id, const socket_handle &aux_socket, ipc::server &factory)
				: _id(id), _aux_socket(aux_socket)
			{	_inbound = factory.create_session(*this);	}

			socket_handler::status session::handle_socket(const socket_handle &s)
			{
				unsigned int size;

				if (recv_scalar(s, size, MSG_WAITALL) > 0)
				{
					_buffer.resize(size);
					::recv(s, reinterpret_cast<char *>(&_buffer[0]), size, MSG_WAITALL);
					_inbound->message(const_byte_range(&_buffer[0], _buffer.size()));
					return socket_handler::proceed;
				}
				else
				{
					_inbound->disconnect();
				}
				return socket_handler::remove_this;
			}

			void session::disconnect() throw()
			{	send_scalar(_aux_socket, _id);	}

			void session::message(const_byte_range /*payload*/)
			{	}


			server::server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
				: _factory(factory), _aux_socket(0), _next_id(100)
			{
				socket_handle server_socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
				host_port hp(endpoint_id);
				sockaddr_in service = { AF_INET, htons(hp.port), inet_addr(hp.host.c_str()), { 0 } };

				setup_socket(server_socket);
				if (::bind(server_socket, (sockaddr *)&service, sizeof(service)))
					throw initialization_failed("bind() failed");
				if (::listen(server_socket, max_backlog))
					throw initialization_failed("listen() failed");

				_aux_socket.reset(connect_aux(hp.port));
				_handlers.push_back(socket_handler::ptr_t(new socket_handler(_next_id++, server_socket,
					bind(&server::accept_preinit, this, _2))));
				_server_thread.reset(new mt::thread(bind(&socket_handler::run<handlers_t>, ref(_handlers))));
			}

			server::~server()
			{
				send_scalar(_aux_socket, 0u);
				_server_thread->join();
			}

			socket_handler::status server::handle_preinit(socket_handler &h, const socket_handle &s)
			{
				unsigned int magic;

				if (sizeof(magic) == recv_scalar(s, magic, MSG_PEEK) && magic == 0xFFFFFFFE)
				{
					recv_scalar(s, magic, MSG_WAITALL);
					for (handlers_t::const_iterator i = _handlers.begin() ; i != _handlers.end(); ++i)
					{
						socket_handler::handler_t &handler = (*i)->handler;

						if (i == _handlers.begin()) // acceptor, guaranteed to be the first
							handler = bind(&server::accept_regular, this, _2);
						else if (i->get() == &h) // aux handler
							handler = bind(&server::handle_aux, this, _2);
						else // regular session handler
							handler = bind(&session::handle_socket, shared_ptr<session>(new session((*i)->id, _aux_socket, *_factory)), _2);
					}
				}
				return socket_handler::proceed;
			}

			socket_handler::status server::accept_preinit(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));

				setup_socket(new_connection);
				_handlers.push_back(socket_handler::ptr_t(new socket_handler(_next_id++, new_connection,
					bind(&server::handle_preinit, this, _1, _2))));
				return socket_handler::proceed;
			}

			socket_handler::status server::accept_regular(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));
				const unsigned id = _next_id++;
				shared_ptr<session> session_(new session(id, _aux_socket, *_factory));

				setup_socket(new_connection);
				_handlers.push_back(socket_handler::ptr_t(new socket_handler(id, new_connection,
					bind(&session::handle_socket, session_, _2))));
				return socket_handler::proceed;
			}

			socket_handler::status server::handle_aux(const socket_handle &s)
			{
				unsigned id;

				if (recv_scalar(s, id, MSG_WAITALL) > 0)
					for (handlers_t::iterator i = _handlers.begin(); i != _handlers.end(); ++i)
						if ((*i)->id == id)
						{
							_handlers.erase(i);
							return socket_handler::proceed;
						}
				return socket_handler::exit;
			}

			int server::connect_aux(unsigned short port)
			{
				sockaddr_in service = { AF_INET, htons(port), inet_addr(c_localhost), { 0 } };
				int s = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
				u_long arg = 1 /* nonblocking */;

				if (-1 == s)
					throw initialization_failed("aux socket creation failed");
				::ioctl(s, O_NONBLOCK, &arg);
				::connect(s, (sockaddr *)&service, sizeof(service));
				send_scalar(s, 0xFFFFFFFE);
 				return s;
			}


			shared_ptr<void> run_server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
			{	return shared_ptr<void>(new server(endpoint_id, factory));}
		}
	}
}
