#pragma once

#include <ipc/endpoint.h>
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
				outbound_channel();

			public:
				bool disconnected;
				std::vector<unsigned int /*module_id*/> requested_metadata;
				std::vector< std::vector<unsigned int /*thread_id*/> > requested_threads;
				unsigned requested_upadtes;

			private:
				virtual void disconnect() throw();
				virtual void message(const_byte_range payload);
			};
		}
	}
}
