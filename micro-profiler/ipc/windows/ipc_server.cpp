#include "../server.h"

#include "common.h"

#include <algorithm>
#include <functional>
#include <wpl/base/concepts.h>
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
			void on_connect();

		private:
			server *_server;
			string _server_name;
			shared_ptr<void> _stop;
			shared_ptr<void> _completion;
			shared_ptr<void> _pipe;
			vector< shared_ptr<server::outer_session> > _sessions;
		};

		class server::outer_session : OVERLAPPED, wpl::noncopyable, public enable_shared_from_this<server::outer_session>
		{
		public:
			outer_session(server::impl &server, const shared_ptr<void> &pipe)
				: _server(server), _pipe(pipe), _inner(server.create_session())
			{	read();	}

		private:
			void read()
			{
				*static_cast<OVERLAPPED*>(this) = zero_init();
				::ReadFileEx(_pipe.get(), _buffer, sizeof(_buffer), this, &on_read);
			}

			void chunk_ready(DWORD /*error_code*/, DWORD bytes_transferred)
			{
				DWORD dummy;

				_input.insert(_input.end(), _buffer, _buffer + bytes_transferred);
				if (::GetOverlappedResult(_pipe.get(), this, &dummy, TRUE))
				{
					_inner->on_message(_input, _output);
					_input.clear();
					*static_cast<OVERLAPPED*>(this) = zero_init();
					::WriteFileEx(_pipe.get(), !_output.empty() ? &_output[0] : 0, static_cast<DWORD>(_output.size()), this, &on_write);
				}
				else /*if (ERROR_MORE_DATA == ::GetLastError())*/
				{
					read();
				}
			}

			static void __stdcall on_read(DWORD error_code, DWORD bytes_transferred, OVERLAPPED *overlapped)
			{
				outer_session *self = static_cast<outer_session *>(overlapped);

				switch (error_code)
				{
				case ERROR_SUCCESS:
					self->chunk_ready(error_code, bytes_transferred);
					break;

				default:
					self->_server.on_session_closed(self->shared_from_this());
				}
			}

			static void __stdcall on_write(DWORD /*error_code*/, DWORD /*bytes_transferred*/, OVERLAPPED *overlapped)
			{	static_cast<outer_session *>(overlapped)->read();	}

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
					on_connect();
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
			::ConnectNamedPipe(pipe.get(), this);
			return pipe;
		}

		void server::impl::on_connect()
		{
			shared_ptr<void> replacement = create_pipe(false);
			shared_ptr<server::outer_session> s(make_shared<server::outer_session>(*this, _pipe));

			_pipe = replacement;
			_sessions.push_back(s);
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
