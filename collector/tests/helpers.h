#pragma once

#include <collector/types.h>
#include <common/file_id.h>
#include <common/module.h>
#include <tuple>
#include <vector>

namespace micro_profiler
{
	class calls_collector;
	class calls_collector_thread;

	namespace tests
	{
		void on_enter(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp,
			const void *callee);
		const void *on_exit(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp);

		void on_enter(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp, const void *callee);
		const void *on_exit(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp);


		struct virtual_stack
		{
			virtual_stack()
				: _stack(1000), _stack_ptr(&_stack[500])
			{	}

			template <typename CollectorT>
			void on_enter(CollectorT &collector, timestamp_t timestamp, const void *callee)
			{	micro_profiler::tests::on_enter(collector, --_stack_ptr, timestamp, callee);	}

			template <typename CollectorT>
			const void *on_exit(CollectorT &collector, timestamp_t timestamp)
			{	return micro_profiler::tests::on_exit(collector, _stack_ptr++, timestamp);	}

			std::vector<const void *> _stack;
			const void **_stack_ptr;
		};

		struct mapping_less
		{
			bool operator ()(const module::mapping_instance &lhs, const module::mapping_instance &rhs) const
			{
				return std::make_tuple(lhs.first, lhs.second.module_id, lhs.second.base, lhs.second.path)
					< std::make_tuple(rhs.first, rhs.second.module_id, rhs.second.base, rhs.second.path);
			}
		};



		template <typename T>
		inline const typename T::value_type *find_module(T &m, const std::string &path)
		{
			for (typename T::const_iterator i = m.begin(); i != m.end(); ++i)
				if (file_id(i->second.path) == file_id(path))
					return &*i;
			return 0;
		}

		template <typename ContainerT, typename KeyT>
		const typename ContainerT::value_type::second_type *find_by_first(const ContainerT &c, const KeyT &key)
		{
			for (typename ContainerT::const_iterator i = c.begin(); i != c.end(); ++i)
				if (i->first == key)
					return &i->second;
			return 0;
		}

		inline module::mapping make_mapping(void *base, std::string path,
			std::vector<mapped_region> regions = std::vector<mapped_region>())
		{
			module::mapping m = {	path, static_cast<byte *>(base), regions };
			return m;
		}

		inline module::mapping_instance make_mapping_instance(id_t mapping_id, id_t module_id, std::string path,
			long_address_t base)
		{
			module::mapping_ex m = {	module_id, path, base, };
			return std::make_pair(mapping_id, m);
		}

		inline const void *addr(size_t value)
		{	return reinterpret_cast<const void *>(value);	}
	}

	inline bool operator ==(const call_record &lhs, const call_record &rhs)
	{	return lhs.timestamp == rhs.timestamp && lhs.callee == rhs.callee;	}
}
