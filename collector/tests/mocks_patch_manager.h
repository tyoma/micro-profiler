#pragma once

#include <patcher/interface.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class patch_manager : public micro_profiler::patch_manager
			{
			public:
				std::function<void (patch_state &states, unsigned int persistent_id)> on_query;
				std::function<void (apply_results &results, unsigned int persistent_id, void *base,
					std::shared_ptr<void> lock, request_range targets)> on_apply;
				std::function<void (revert_results &results, unsigned int persistent_id, request_range targets)> on_revert;

			private:
				virtual void query(patch_state &states, unsigned int persistent_id) override;
				virtual void apply(apply_results &results, unsigned int persistent_id, void *base,
					std::shared_ptr<void> lock, request_range targets) override;
				virtual void revert(revert_results &results, unsigned int persistent_id, request_range targets) override;
			};



			inline void patch_manager::query(patch_state &states, unsigned int persistent_id)
			{	on_query(states, persistent_id);	}

			inline void patch_manager::apply(apply_results &results, unsigned int persistent_id, void *base,
				std::shared_ptr<void> lock, request_range targets)
			{	on_apply(results, persistent_id, base, lock, targets);	}

			inline void patch_manager::revert(revert_results &results, unsigned int persistent_id, request_range targets)
			{	on_revert(results, persistent_id, targets);	}
		}
	}
}
