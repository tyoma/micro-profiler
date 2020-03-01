#pragma once

#include <collector/calls_collector.h>
#include <common/file_id.h>
#include <common/module.h>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		inline void on_enter(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp,
			const void *callee)
		{	collector.on_enter(stack_ptr, timestamp, callee);	}

		inline const void *on_exit(calls_collector_thread &collector, const void **stack_ptr, timestamp_t timestamp)
		{	return collector.on_exit(stack_ptr, timestamp);	}

		inline void on_enter(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp, const void *callee)
		{	calls_collector::on_enter(&collector, stack_ptr, timestamp, callee);	}

		inline const void *on_exit(calls_collector &collector, const void **stack_ptr, timestamp_t timestamp)
		{	return calls_collector::on_exit(&collector, stack_ptr, timestamp);	}


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

		template <typename T>
		inline const typename T::value_type *find_module(T &m, const std::string &path)
		{
			for (typename T::const_iterator i = m.begin(); i != m.end(); ++i)
				if (file_id(i->path) == file_id(path))
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

		inline const void *addr(size_t value)
		{	return reinterpret_cast<const void *>(value);	}
	}

	inline bool operator ==(const call_record &lhs, const call_record &rhs)
	{	return lhs.timestamp == rhs.timestamp && lhs.callee == rhs.callee;	}
}
