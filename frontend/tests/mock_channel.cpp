#include "mock_channel.h"

#include <common/serialization.h>
#include <common/stream.h>
#include <functional>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			outbound_channel::outbound_channel()
				: disconnected(false), requested_upadtes(0u)
			{	}

			void outbound_channel::disconnect() throw()
			{	disconnected = true;	}

			void outbound_channel::message(const_byte_range payload)
			{
				buffer_reader reader(payload);
				strmd::deserializer<buffer_reader, packer> d(reader);
				messages_id c;
				unsigned token;

				switch (d(c), d(token), c)
				{
				case request_update:
					requested_upadtes++;
					break;

				case request_module_metadata:
					requested_metadata.push_back(0), d(requested_metadata.back());
					break;

				case request_threads_info:
					requested_threads.push_back(std::vector<unsigned>()), d(requested_threads.back());
					break;

				default:
					break;
				}
			}
		}
	}
}
