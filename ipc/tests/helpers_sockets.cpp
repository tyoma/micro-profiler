#include "helpers_sockets.h"

#include <arpa/inet.h>
#include <common/noncopyable.h>
#include <ipc/endpoint_sockets.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			socket_handle::socket_handle()
				: _socket(static_cast<int>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)))
			{	}

			socket_handle::~socket_handle()
			{	::close(_socket);	}

			socket_handle::operator int() const
			{	return _socket;	}


			sender::sender(unsigned short port)
			{
				sockaddr_in service = { AF_INET, htons(port), inet_addr("127.0.0.1"), { 0 } };

				::connect(_socket, (sockaddr *)&service, sizeof(service));
			}

			bool sender::operator ()(const void *buffer, size_t sz)
			{
				int size_ = static_cast<unsigned int>(sz);
				sockets::byte_representation<unsigned int> size;

				size.value = size_;
				size.reorder();
				if (::send(_socket, size.bytes, sizeof(size.bytes), MSG_NOSIGNAL) < (int)sizeof(size.bytes))
					return false;
				if (::send(_socket, static_cast<const char *>(buffer), size_, MSG_NOSIGNAL) < size_)
					return false;
				return true;
			}


			reader::reader(unsigned short port)
			{
				sockaddr_in service = { AF_INET, htons(port), inet_addr("127.0.0.1"), { 0 } };

				::connect(_socket, (sockaddr *)&service, sizeof(service));
			}

			bool reader::operator ()(vector<byte> &buffer)
			{
				sockets::byte_representation<unsigned int> size;

				if (::recv(_socket, size.bytes, sizeof(size.bytes), MSG_WAITALL) < (int)sizeof(size.bytes))
					return false;
				buffer.resize(size.value);
				if (::recv(_socket, reinterpret_cast<char *>(&buffer[0]), size.value, MSG_WAITALL) < (int)size.value)
					return false;
				return true;
			}


			bool is_local_port_open(unsigned short port)
			{
				socket_handle s;
				sockaddr_in service = { AF_INET, htons(port), inet_addr("127.0.0.1"), { 0 } };

				return 0 == ::connect(s, (sockaddr *)&service, sizeof(service));
			}
		}
	}
}
