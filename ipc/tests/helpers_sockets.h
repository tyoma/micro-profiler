#pragma once

#include <common/noncopyable.h>
#include <common/types.h>
#include <vector>

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			class socket_handle : noncopyable
			{
			public:
				socket_handle();
				~socket_handle();

				operator int() const;

			private:
				int _socket;
			};

			class sender : noncopyable
			{
			public:
				sender(unsigned short port);

				bool operator ()(const void *buffer, size_t size);				

			private:
				socket_handle _socket;
			};

			class reader : noncopyable
			{
			public:
				reader(unsigned short port);

				bool operator ()(std::vector<byte> &buffer);				

			private:
				socket_handle _socket;
			};



			bool is_local_port_open(unsigned short port);
		}
	}
}
