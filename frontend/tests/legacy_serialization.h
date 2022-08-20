#pragma once

#include <collector/primitives.h>
#include <frontend/database.h>
#include <strmd/version.h>

namespace micro_profiler
{
	namespace tests
	{
		class vector_adapter;
		typedef std::pair<long_address_t, unsigned> legacy_function_key;


		struct file_v3_components
		{
			typedef call_graph_types<long_address_t> statistic_types;

			timestamp_t ticks_per_second;
			containers::unordered_map<unsigned int /*instance_id*/, module::mapping_ex> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> modules;
			std::vector< std::pair<statistic_types::key, statistic_types::node> > statistics;
		};

		struct file_v4_components
		{
			timestamp_t ticks_per_second;
			containers::unordered_map<unsigned int /*instance_id*/, module::mapping_ex> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> modules;
			std::vector< std::pair<legacy_function_key, call_graph_types<legacy_function_key>::node> > statistics;
			containers::unordered_map<unsigned int, thread_info> threads;
		};

		struct file_v5_components
		{
			initialization_data process_info;
			containers::unordered_map<unsigned int /*instance_id*/, module::mapping_ex> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> modules;
			calls_statistics_table statistics;
			containers::unordered_map<unsigned int, thread_info> threads;
		};

		struct file_v6_components
		{
			initialization_data process_info;
			std::vector<tables::module_mapping> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, module_info_metadata> modules;
			calls_statistics_table statistics;
			std::vector<tables::thread> threads;
		};

		// This is always the up-to-date serialization scheme.
		struct file_components
		{
			initialization_data process_info;
			std::vector<tables::module_mapping> mappings;
			std::vector<tables::module> modules;
			calls_statistics_table statistics;
			std::vector<tables::thread> threads;
		};



		void serialize_legacy(vector_adapter &buffer, const file_v3_components &components);
		void serialize_legacy(vector_adapter &buffer, const file_v4_components &components);
		void serialize_legacy(vector_adapter &buffer, const file_v5_components &components);
		void serialize_legacy(vector_adapter &buffer, const file_v6_components &components);
	}
}

namespace strmd
{
	template <> struct version<micro_profiler::tests::file_v5_components> {	enum {	value = 5	};	};
	template <> struct version<micro_profiler::tests::file_v6_components> {	enum {	value = 6	};	};
}
