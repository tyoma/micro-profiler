#pragma once

#include <frontend/profiling_cache.h>
#include <functional>
#include <map>
#include <memory>
#include <scheduler/task.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			struct profiling_cache_tasks : micro_profiler::profiling_cache_tasks
			{
				virtual scheduler::task<id_t> persisted_module_id(unsigned hash) override;

				std::map< unsigned /*hash*/, std::shared_ptr< scheduler::task_node<id_t> > > tasks;
			};

			class profiling_cache : public micro_profiler::profiling_cache
			{
			public:
				virtual std::shared_ptr<module_info_metadata> load_metadata(unsigned int hash) override;
				virtual void store_metadata(const module_info_metadata &metadata) override;

				virtual std::vector<tables::cached_patch> load_default_patches(id_t cached_module_id) override;
				virtual void update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
					std::vector<unsigned int> remove_rva) override;

			public:
				std::function<std::shared_ptr<module_info_metadata> (unsigned hash)> on_load_metadata;
				std::function<void (const module_info_metadata &metadata)> on_store_metadata;
				std::function<std::vector<tables::cached_patch> (id_t cached_module_id)> on_load_default_patches;
				std::function<void (id_t cached_module_id, std::vector<unsigned int> add_rva,
					std::vector<unsigned int> remove_rva)> on_update_default_patches;
			};

			struct profiling_cache_with_tasks : profiling_cache_tasks, profiling_cache
			{
			};



			inline scheduler::task<id_t> profiling_cache_tasks::persisted_module_id(unsigned hash)
			{
				const auto i = tasks.find(hash);

				if (i != tasks.end())
					return scheduler::task<id_t>(std::shared_ptr< scheduler::task_node<id_t> >(i->second));
				auto n = std::make_shared< scheduler::task_node<id_t> >();
				tasks.insert(std::make_pair(hash, n));
				return scheduler::task<id_t>(std::move(n));
			}

			inline std::shared_ptr<module_info_metadata> profiling_cache::load_metadata(unsigned int hash)
			{	return on_load_metadata ? on_load_metadata(hash) : nullptr;	}

			inline void profiling_cache::store_metadata(const module_info_metadata &metadata)
			{	on_store_metadata(metadata);	}

			inline std::vector<tables::cached_patch> profiling_cache::load_default_patches(id_t cached_module_id)
			{
				return on_load_default_patches
					? on_load_default_patches(cached_module_id) : std::vector<tables::cached_patch>();
			}

			inline void profiling_cache::update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
				std::vector<unsigned int> remove_rva)
			{	on_update_default_patches(cached_module_id, add_rva, remove_rva);	}
		}
	}
}
