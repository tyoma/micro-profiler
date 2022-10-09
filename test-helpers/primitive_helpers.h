#pragma once

#include <common/primitives.h>

namespace micro_profiler
{
	namespace tests
	{
		template <typename AddressT>
		inline std::pair<AddressT, typename call_graph_types<AddressT>::node> make_statistics(AddressT address,
			count_t times_called, timestamp_t inclusive_time, timestamp_t exclusive_time)
		{
			return std::make_pair(address, function_statistics(times_called, inclusive_time, exclusive_time));
		}

		template <typename AddressT>
		inline std::pair< AddressT, typename call_graph_types<AddressT>::node > make_statistics(AddressT address,
			count_t times_called, timestamp_t inclusive_time, timestamp_t exclusive_time,
			std::vector< std::pair<AddressT, typename call_graph_types<AddressT>::node> > callees)
		{
			auto r = make_statistics(address, times_called, inclusive_time, exclusive_time);

			r.second.callees = typename call_graph_types<AddressT>::nodes_map(callees.begin(), callees.end());
			return r;
		}

		template <typename AddressT>
		inline std::pair<AddressT, typename call_graph_types<AddressT>::node> make_hstatistics(AddressT address,
			const scale_t &inclusive_scale, const scale_t &exclusive_scale)
		{
			function_statistics s;

			s.inclusive.set_scale(inclusive_scale);
			s.exclusive.set_scale(exclusive_scale);
			return std::make_pair(address, s);
		}

		template <typename AddressT>
		inline std::pair<AddressT, typename call_graph_types<AddressT>::node> make_hstatistics(AddressT address,
			const scale_t &inclusive_scale, const scale_t &exclusive_scale,
			std::vector< std::pair<AddressT, typename call_graph_types<AddressT>::node> > callees)
		{
			auto r = make_hstatistics(address, inclusive_scale, exclusive_scale);

			r.second.callees = typename call_graph_types<AddressT>::nodes_map(callees.begin(), callees.end());
			return r;
		}
	}
}
