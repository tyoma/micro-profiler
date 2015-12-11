#include "../server.h"

#include "common.h"

#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		class server::impl : OVERLAPPED
		{
		public:
			impl(const char *server_name, const server::callback &callback)
				: _callback(callback), _pipe(::CreateNamedPipeA((c_pipe_ns + c_prefix + server_name).c_str(),
					PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
					1, 1000, 1000, 0, NULL), &::CloseHandle)
			{
				LARGE_INTEGER li_period = { 0 };

				*static_cast<OVERLAPPED*>(this) = zero_init();
				::ConnectNamedPipe(_pipe.get(), this);

				_connect_timer.reset(::CreateWaitableTimer(NULL, FALSE, NULL), &reset_timer);
				::SetWaitableTimer(_connect_timer.get(), &li_period, 50, &on_connection_check, this, FALSE);
			}

			~impl()
			{	::CancelIoEx(_pipe.get(), NULL);	}

		private:
			void read()
			{
				*static_cast<OVERLAPPED*>(this) = zero_init();
				::ReadFileEx(_pipe.get(), _buffer, sizeof(_buffer), this, &read_io_completion);
			}

			void chunk_ready(DWORD /*error_code*/, DWORD bytes_transferred)
			{
				DWORD dummy;

				_input_message.insert(_input_message.end(), _buffer, _buffer + bytes_transferred);
				if (GetOverlappedResult(_pipe.get(), this, &dummy, FALSE))
				{
					_callback(_input_message, _output_message);
					_input_message.clear();
					*static_cast<OVERLAPPED*>(this) = zero_init();
					::WriteFileEx(_pipe.get(), _output_message.empty() ? NULL : &_output_message[0],
						static_cast<DWORD>(_output_message.size()), this, &write_io_completion);
				}
				else /*if (ERROR_MORE_DATA == ::GetLastError())*/
				{
					read();
				}
			}

			static void __stdcall on_connection_check(void *self_, DWORD /*ignored*/, DWORD /*ignored*/)
			{
				impl *self = static_cast<impl *>(self_);

				if (HasOverlappedIoCompleted(self))
				{
					self->_connect_timer.reset();
					self->read();
				}
			}

			static void __stdcall read_io_completion(DWORD error_code, DWORD bytes_transferred, OVERLAPPED *overlapped)
			{	static_cast<impl *>(overlapped)->chunk_ready(error_code, bytes_transferred);	}

			static void __stdcall write_io_completion(DWORD /*error_code*/, DWORD /*bytes_transferred*/, OVERLAPPED *overlapped)
			{	static_cast<impl *>(overlapped)->read();	}

			static void reset_timer(HANDLE htimer)
			{
				::CancelWaitableTimer(htimer);
				::CloseHandle(htimer);
			}

		private:
			vector<byte> _input_message, _output_message;
			shared_ptr<void> _pipe, _connect_timer;
			server::callback _callback;
			byte _buffer[10000];
		};


		server::server(const char *server_name, const callback &cb)
			: _impl(new impl(server_name, cb))
		{	}

		server::~server()
		{	}
	}
}
