#pragma once

#include <frontend/profiling_cache.h>
#include <functional>
#include <memory>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class profiling_cache : public micro_profiler::profiling_cache
			{
			public:
				virtual std::shared_ptr<module_info_metadata> load_metadata(unsigned int hash,
					id_t associated_module_id) override;
				virtual void store_metadata(const module_info_metadata &metadata, id_t associated_module_id) override;

				virtual std::vector<tables::cached_patch> load_default_patches(id_t cached_module_id) override;
				virtual void update_default_patches(id_t cached_module_id, std::vector<unsigned int> add_rva,
					std::vector<unsigned int> remove_rva) override;

			public:
				std::function<std::shared_ptr<module_info_metadata> (unsigned hash, id_t associated_module_id)>
					on_load_metadata;
				std::function<void (const module_info_metadata &metadata, id_t associated_module_id)> on_store_metadata;
				std::function<std::vector<tables::cached_patch> (id_t cached_module_id)> on_load_default_patches;
				std::function<void (id_t cached_module_id, std::vector<unsigned int> add_rva,
					std::vector<unsigned int> remove_rva)> on_update_default_patches;
			};



			inline std::shared_ptr<module_info_metadata> profiling_cache::load_metadata(unsigned int hash,
				id_t associated_module_id)
			{	return on_load_metadata ? on_load_metadata(hash, associated_module_id) : nullptr;	}

			inline void profiling_cache::store_metadata(const module_info_metadata &metadata, id_t associated_module_id)
			{	on_store_metadata(metadata, associated_module_id);	}

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
