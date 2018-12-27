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

#include "socket_helpers.h"

#include <arpa/inet.h>
#include <common/noncopyable.h>
#include <fcntl.h>
#include <list>
#include <mt/thread.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <vector>

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
			const int c_max_backlog = 5;

			class socket_processor : noncopyable
			{
			public:
				typedef function<bool (socket_processor &self, const socket_handle &s)> handler_t;
				typedef shared_ptr<socket_processor> ptr_t;

			public:
				socket_processor(socket_handle &s, const handler_t &initial_handler);

				template <typename ContainerT>
				static void select_execute(ContainerT &processors);

			public:
				handler_t handler;

			private:
				socket_handle _socket;
			};


			class session : /*outbound*/ ipc::channel, noncopyable
			{
			public:
				session(const socket_handle &aux_socket, ipc::server &factory);

				bool handle_socket(const socket_handle &s);

				void disconnect() throw();
				void message(const_byte_range payload);

			public:
				const void *cancel_cookie;

			private:
				const socket_handle &_aux_socket;
				shared_ptr<ipc::channel> _inbound;
				vector<byte> _buffer;
			};


			class server
			{
			public:
				server(const char *endpoint_id, const shared_ptr<ipc::server> &factory);
				~server();

			private:
				typedef list< shared_ptr<socket_processor> > processors_t;

			private:
				void worker();
				bool handle_pre_init(socket_processor &p, const socket_handle &s);
				bool accept_pre_init(const socket_handle &s);
				bool accept_regular(const socket_handle &s);
				bool handle_aux(const socket_handle &s);

				static int connect_aux(unsigned short port);

			private:
				sockets_initializer _initializer;
				shared_ptr<ipc::server> _factory;
				socket_handle _server_socket, _aux_socket;
				bool _exit;
				shared_ptr<socket_processor> _acceptor;
				processors_t _processors;
				auto_ptr<mt::thread> _server_thread;
			};




			socket_processor::socket_processor(socket_handle &s, const handler_t &initial_handler)
				: handler(initial_handler), _socket(s)
			{	}

			template <typename ContainerT>
			void socket_processor::select_execute(ContainerT &processors)
			{
				fd_set fds;

				FD_ZERO(&fds);
				for (typename ContainerT::const_iterator i = processors.begin(); i != processors.end(); ++i)
					FD_SET((*i)->_socket, &fds);					
				if (::select(FD_SETSIZE, &fds, NULL, NULL, NULL) < 0)
					throw 0; // TODO: handle this
				for (typename ContainerT::iterator i = processors.begin(); i != processors.end(); )
				{
					if (FD_ISSET((*i)->_socket, &fds) && !(*i)->handler(**i, (*i)->_socket))
						i = processors.erase(i);
					else
						++i;
				}
			}


			session::session(const socket_handle &aux_socket, ipc::server &factory)
				: _aux_socket(aux_socket)
			{	_inbound = factory.create_session(*this);	}

			bool session::handle_socket(const socket_handle &s)
			{
				byte_representation<unsigned int> size = { };

				if (::recv(s, size.bytes, sizeof(size), MSG_WAITALL) > 0)
				{
					size.reorder();
					_buffer.resize(size.value);
					::recv(s, reinterpret_cast<char *>(&_buffer[0]), size.value, MSG_WAITALL);
					_inbound->message(const_byte_range(&_buffer[0], _buffer.size()));
					return true;
				}
				else
				{
					_inbound->disconnect();
				}
				return false;
			}

			void session::disconnect() throw()
			{
				byte_representation<const void *> data;

				data.value = cancel_cookie;
				::send(_aux_socket, data.bytes, sizeof(data), 0);
			}

			void session::message(const_byte_range /*payload*/)
			{	}


			server::server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
				: _factory(factory), _server_socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)),
					_aux_socket(0), _exit(false)
			{
				host_port hp(endpoint_id);
				sockaddr_in service = { AF_INET, htons(hp.port), inet_addr(hp.host.c_str()), { 0 } };

				if (::bind(_server_socket, (sockaddr *)&service, sizeof(service)))
					throw initialization_failed("bind() failed");
				if (::listen(_server_socket, c_max_backlog))
					throw runtime_error("listen() failed");
				_aux_socket.reset(connect_aux(hp.port));
				_acceptor.reset(new socket_processor(_server_socket, bind(&server::accept_pre_init, this, _2)));
				_processors.push_back(_acceptor);
				_server_thread.reset(new mt::thread(bind(&server::worker, this)));
			}

			void server::worker()
			{
				do
					socket_processor::select_execute(_processors);
				while (!_exit);
			}

			bool server::handle_pre_init(socket_processor &p, const socket_handle &s)
			{
				byte_representation<unsigned int> data;

				if (sizeof(data) == ::recv(s, data.bytes, sizeof(data), MSG_PEEK)
					&& (data.reorder(), data.value == 0xFFFFFFFE))
				{
					::recv(s, data.bytes, sizeof(data), MSG_WAITALL);
					for (processors_t::const_iterator i = _processors.begin(); i != _processors.end(); ++i)
					{
						if (i->get() != &p && *i != _acceptor)
						{
							shared_ptr<session> session_(new session(_aux_socket, *_factory));

							(*i)->handler = bind(&session::handle_socket, session_, _2);
							session_->cancel_cookie = i->get();
						}
					}
					_acceptor->handler = bind(&server::accept_regular, this, _2);
					p.handler = bind(&server::handle_aux, this, _2);
				}
				return true;
			}

			bool server::accept_pre_init(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));
				socket_processor::ptr_t p(new socket_processor(new_connection, bind(&server::handle_pre_init, this, _1,
					_2)));
				
				_processors.push_back(p);
				return true;
			}

			bool server::accept_regular(const socket_handle &s)
			{
				socket_handle new_connection(::accept(s, NULL, NULL));
				shared_ptr<session> session_(new session(_aux_socket, *_factory));
				socket_processor::ptr_t p(new socket_processor(new_connection, bind(&session::handle_socket, session_,
					_2)));

				session_->cancel_cookie = p.get();
				_processors.push_back(p);
				return true;
			}

			bool server::handle_aux(const socket_handle &s)
			{
				byte_representation<const void *> data;

				if (::recv(s, data.bytes, sizeof(data), MSG_WAITALL) == sizeof(data))
				{
					for (processors_t::iterator i = _processors.begin(); i != _processors.end(); ++i)
						if (i->get() == data.value)
						{
							_processors.erase(i);
							break;
						}
				}
				else
				{
					_exit = true;
				}
				return true;
			}

			server::~server()
			{
				_aux_socket.reset();
				_server_thread->join();
			}

			int server::connect_aux(unsigned short port)
			{
				sockaddr_in service = { AF_INET, htons(port), inet_addr(c_localhost), { 0 } };
				int s = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
				u_long arg = 1 /* nonblocking */;
				byte_representation<unsigned int> data;

				data.value = 0xFFFFFFFE;
				data.reorder();
				if (-1 == s)
					throw initialization_failed("aux socket creation failed");
				::ioctl(s, O_NONBLOCK, &arg);
				::connect(s, (sockaddr *)&service, sizeof(service));
				::send(s, data.bytes, sizeof(data), 0);
 				return s;
			}


			shared_ptr<void> run_server(const char *endpoint_id, const shared_ptr<ipc::server> &factory)
			{	return shared_ptr<void>(new server(endpoint_id, factory));}
		}
	}
}
