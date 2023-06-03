#pragma once

#include <common/compiler.h>
#include <functional>
#include <list>
#include <patcher/interface.h>
#include <tuple>
#include <vector>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			struct call_record
			{
				timestamp_t timestamp;
				const void *callee;

				bool operator ==(const call_record &rhs) const;
			};


			struct trace_events
			{
				static void CC_(fastcall) on_enter(trace_events *self, const void **stack_ptr,
					timestamp_t timestamp, const void *callee) _CC(fastcall);
				static const void *CC_(fastcall) on_exit(trace_events *self, const void **stack_ptr,
					timestamp_t timestamp) _CC(fastcall);

				std::vector<call_record> call_log;
				std::vector< std::pair<const void *, const void **> > return_stack;

				std::vector<const void **> enter_stack_addresses;
				std::vector<const void **> exit_stack_addresses;
			};


			class mapping_access : public micro_profiler::mapping_access
			{
			public:
				mapping_access();

			public:
				std::function<std::shared_ptr<module::mapping> (id_t mapping_id)> on_lock_mapping;
				events *subscription;

			private:
				virtual std::shared_ptr<module::mapping> lock_mapping(id_t mapping_id) override;
				virtual std::shared_ptr<void> notify(events &events_) override;
			};


			class memory_manager : public micro_profiler::memory_manager
			{
			public:
				typedef std::tuple<byte_range, int /*scoped*/, int /*released*/> lock_info;

			public:
				std::vector<lock_info> locks() const;

			public:
				std::vector< std::tuple<std::shared_ptr<executable_memory_allocator>, const_byte_range, std::size_t> >
					allocators;

			private:
				virtual std::shared_ptr<executable_memory_allocator> create_executable_allocator(const_byte_range,
					std::ptrdiff_t) override;

				virtual std::shared_ptr<void> scoped_protect(byte_range region, int scoped_protection,
					int released_protection) override;

			private:
				std::list<lock_info> _locks;
			};


			class patch : public micro_profiler::patch
			{
			public:
				patch(std::function<void (void *target, int act)> on_patch_action, void *target);
				virtual ~patch();

				virtual bool activate() override;
				virtual bool revert() override;

			private:
				const std::function<void (void *target, int act)> _on_patch_action;
				void *_target;
			};



			inline bool call_record::operator ==(const call_record &rhs) const
			{	return callee == rhs.callee;	}
		}
	}
}
