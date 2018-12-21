#pragma once

#include <common/noncopyable.h>
#include <mt/thread.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			class socket_h : noncopyable
			{
			public:
				socket_h();
				~socket_h();

				operator int() const;

			private:
				int _socket;
			};

			class sender : noncopyable
			{
			public:
				sender(unsigned short port);
				~sender();

				bool operator ()(const void *buffer, size_t size);

			private:
				socket_h _socket;
			};



			bool is_local_port_open(unsigned short port);
		}
	}
}
