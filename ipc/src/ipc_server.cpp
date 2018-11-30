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

#include <ipc/server.h>

#include "common.h"

#include <algorithm>
#include <common/noncopyable.h>
#include <functional>
#include <memory>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		class server::impl : OVERLAPPED
		{
		public:
			impl(const char *server_name, server *server_);
			~impl();

			void run();
			void stop();

			server::session *create_session();
			void on_session_closed(const shared_ptr<server::outer_session> &session);

		private:
			shared_ptr<void> create_pipe(bool first);
			void on_connected();

		private:
			server *_server;
			string _server_name;
			shared_ptr<void> _stop;
			shared_ptr<void> _completion;
			shared_ptr<void> _pipe;
			vector< shared_ptr<server::outer_session> > _sessions;
		};

		class server::outer_session : OVERLAPPED, noncopyable, public enable_shared_from_this<server::outer_session>
		{
		public:
			outer_session(server::impl &server, const shared_ptr<void> &pipe);
			~outer_session();

		private:
			void read();
			void on_chunk_ready(DWORD /*error_code*/, DWORD bytes_transferred);

		private:
			server::impl &_server;
			shared_ptr<void> _pipe;
			auto_ptr<server::session> _inner;
			vector<byte> _input, _output;
			byte _buffer[1000];
		};



		server::impl::impl(const char *server_name, server *server_)
			: _server(server_), _server_name(c_pipe_ns + c_prefix + server_name),
				_stop(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle),
				_completion(::CreateEvent(NULL, TRUE, FALSE, NULL), &::CloseHandle),
				_pipe(create_pipe(true))
		{	}

		server::impl::~impl()
		{	::CancelIoEx(_pipe.get(), NULL);	}

		void server::impl::run()
		{
			HANDLE handles[] = { _stop.get(), _completion.get(), };

			for (;;)
				switch (::WaitForMultipleObjectsEx(_countof(handles), handles, FALSE, INFINITE, TRUE))
				{
				case WAIT_IO_COMPLETION:
					break;

				case WAIT_OBJECT_0:
					return;

				case WAIT_OBJECT_0 + 1:
					DWORD dummy;

					::GetOverlappedResult(_pipe.get(), this, &dummy, TRUE);
					on_connected();
				}
		}

		void server::impl::stop()
		{	::SetEvent(_stop.get());	}

		void server::impl::on_session_closed(const shared_ptr<server::outer_session> &session)
		{	_sessions.erase(remove(_sessions.begin(), _sessions.end(), session), _sessions.end());	}

		server::session *server::impl::create_session()
		{	return _server->create_session();	}

		shared_ptr<void> server::impl::create_pipe(bool first)
		{
			shared_ptr<void> pipe(::CreateNamedPipeA(_server_name.c_str(),
				PIPE_ACCESS_DUPLEX | (first ? FILE_FLAG_FIRST_PIPE_INSTANCE : 0) | FILE_FLAG_OVERLAPPED,
				PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, PIPE_UNLIMITED_INSTANCES, 1000, 1000, 0, NULL),
				&::CloseHandle);

			*static_cast<OVERLAPPED*>(this) = zero_init();
			hEvent = _completion.get();
			::ResetEvent(hEvent);
			switch (::ConnectNamedPipe(pipe.get(), this), ::GetLastError())
			{
			case ERROR_PIPE_CONNECTED:
				::SetEvent(hEvent); // A client managed to sneak into the interval between Create* and Connect*.

			case ERROR_IO_PENDING:
				break;

			default:
				// Error handling goes here...
				break;
			}

			return pipe;
		}

		void server::impl::on_connected()
		{
			shared_ptr<void> replacement = create_pipe(false);
			shared_ptr<server::outer_session> outer(new server::outer_session(*this, _pipe));

			_sessions.push_back(outer);
			_pipe = replacement;
		}


		server::outer_session::outer_session(server::impl &server, const shared_ptr<void> &pipe)
			: _server(server), _pipe(pipe), _inner(server.create_session())
		{	read();	}

		server::outer_session::~outer_session()
		{	}

		void server::outer_session::read()
		{
			struct read_completion
			{
				static void __stdcall routine(DWORD error_code, DWORD bytes_transferred, OVERLAPPED *overlapped)
				{
					outer_session *self = static_cast<outer_session *>(overlapped);

					switch (error_code)
					{
					case ERROR_SUCCESS:
						self->on_chunk_ready(error_code, bytes_transferred);
						break;

					default:
						self->_server.on_session_closed(self->shared_from_this());
					}
				}
			};

			*static_cast<OVERLAPPED*>(this) = zero_init();
			::ReadFileEx(_pipe.get(), _buffer, sizeof(_buffer), this, &read_completion::routine);
		}

		void server::outer_session::on_chunk_ready(DWORD /*error_code*/, DWORD bytes_transferred)
		{
			struct write_completion
			{
				static void __stdcall routine(DWORD /*error_code*/, DWORD /*bytes_transferred*/, OVERLAPPED *overlapped)
				{	static_cast<outer_session *>(overlapped)->read();	}
			};

			DWORD dummy;

			_input.insert(_input.end(), _buffer, _buffer + bytes_transferred);
			if (::GetOverlappedResult(_pipe.get(), this, &dummy, TRUE))
			{
				_inner->on_message(_input, _output);
				_input.clear();
				*static_cast<OVERLAPPED*>(this) = zero_init();
				::WriteFileEx(_pipe.get(), !_output.empty() ? &_output[0] : 0, static_cast<DWORD>(_output.size()), this,
					&write_completion::routine);
			}
			else /*if (ERROR_MORE_DATA == ::GetLastError())*/
			{
				read();
			}
		}


		server::server(const char *server_name)
		{	_impl = new impl(server_name, this);	}

		server::~server()
		{	delete _impl;	}

		void server::run()
		{	_impl->run();	}

		void server::stop()
		{	_impl->stop();	}
	}
}
