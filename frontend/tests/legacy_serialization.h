#pragma once

#include <frontend/tables.h>

namespace micro_profiler
{
	namespace tests
	{
		class vector_adapter;

		struct file_v3_components
		{
			typedef statistic_types_t<long_address_t> statistic_types;

			timestamp_t ticks_per_second;
			containers::unordered_map<unsigned int /*instance_id*/, mapped_module_identified> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, tables::module_info> modules;
			containers::unordered_map<statistic_types::key, statistic_types::function_detailed> statistics;
		};

		struct file_v4_components
		{
			timestamp_t ticks_per_second;
			containers::unordered_map<unsigned int /*instance_id*/, mapped_module_identified> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, tables::module_info> modules;
			containers::unordered_map<statistic_types::key, statistic_types::function_detailed> statistics;
			containers::unordered_map<unsigned int, thread_info> threads;
		};

		// This is always the up-to-date serialization scheme.
		struct file_components
		{
			initialization_data process_info;
			containers::unordered_map<unsigned int /*instance_id*/, mapped_module_identified> mappings;
			containers::unordered_map<unsigned int /*persistent_id*/, tables::module_info> modules;
			containers::unordered_map<statistic_types::key, statistic_types::function_detailed> statistics;
			containers::unordered_map<unsigned int, thread_info> threads;
			containers::unordered_map<unsigned int, tables::patches> patches;
		};



		void serialize_legacy(vector_adapter &buffer, const file_v3_components &components);
		void serialize_legacy(vector_adapter &buffer, const file_v4_components &components);
	}
}
