#pragma once

#include <common/serialization.h>
#include <functional>
#include <ipc/endpoint.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class outbound_channel : public ipc::channel
			{
			public:
				outbound_channel()
					: disconnected(false), requested_upadtes(0u)
				{	}

			public:
				bool disconnected;
				std::vector<unsigned int /*persistent_id*/> requested_metadata;
				std::vector< std::vector<unsigned int /*thread_id*/> > requested_threads;
				unsigned requested_upadtes;

			private:
				virtual void disconnect() throw()
				{	disconnected = true;	}

				virtual void message(const_byte_range payload)
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
			};
		}
	}
}
