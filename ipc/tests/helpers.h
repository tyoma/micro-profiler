#pragma once

#include <common/pod_vector.h>
#include <common/serialization.h>
#include <ipc/endpoint.h>
#include <strmd/serializer.h>

namespace micro_profiler
{
	namespace ipc
	{
		template <typename PayloadT>
		inline void send_standard(ipc::channel &c, int id, unsigned long long token, const PayloadT &payload)
		{
			pod_vector<byte> data;
			buffer_writer< pod_vector<byte> > w(data);
			strmd::serializer<buffer_writer< pod_vector<byte> >, packer> s(w);

			s(id);
			s(token);
			s(payload);
			c.message(const_byte_range(data.data(), data.size()));
		}

		template <typename PayloadT>
		inline void send_message(ipc::channel &c, int id, const PayloadT &payload)
		{
			pod_vector<byte> data;
			buffer_writer< pod_vector<byte> > w(data);
			strmd::serializer<buffer_writer< pod_vector<byte> >, packer> s(w);

			s(id);
			s(payload);
			c.message(const_byte_range(data.data(), data.size()));
		}
	}
}
