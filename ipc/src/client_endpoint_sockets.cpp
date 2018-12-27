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
#include <netinet/in.h>
#include <stdlib.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace sockets
		{
			class client_session : public channel
			{
			public:
				client_session(const host_port &hp, channel &inbound);

				virtual void disconnect() throw();
				virtual void message(const_byte_range payload);

			private:
				static int open(const host_port &hp);

			private:
				sockets_initializer _initializer;
				socket_handle _socket;
			};



			client_session::client_session(const host_port &hp, channel &/*inbound*/)
				: _socket(open(hp))
			{	}

			void client_session::disconnect() throw()
			{	}

			void client_session::message(const_byte_range payload)
			{
				unsigned int size_ = static_cast<unsigned int>(payload.length());
				sockets::byte_representation<unsigned int> size;

				size.value = size_;
				size.reorder();
				::send(_socket, size.bytes, sizeof(size.bytes), MSG_NOSIGNAL);
				::send(_socket, reinterpret_cast<const char *>(payload.begin()), size_, MSG_NOSIGNAL);
			}

			int client_session::open(const host_port &hp)
			{
				sockaddr_in service = { AF_INET, htons(hp.port), inet_addr(hp.host.c_str()), { 0 } };
				int hsocket = static_cast<int>(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));

				if (-1 == hsocket)
					throw initialization_failed("socket creation failed");
				if (::connect(hsocket, (sockaddr *)&service, sizeof(service)))
				{
					::close(hsocket);
					throw connection_refused(hp.host.c_str());
				}
				return hsocket;
			}


			shared_ptr<channel> connect_client(const char *destination_endpoint_id, channel &inbound)
			{
				host_port hp(destination_endpoint_id);

				return shared_ptr<channel>(new client_session(hp, inbound));
			}
		}
	}
}
