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
				std::function<void (patch_state &states, unsigned int module_id)> on_query;
				std::function<void (apply_results &results, unsigned int module_id, void *base,
					std::shared_ptr<void> lock, request_range targets)> on_apply;
				std::function<void (revert_results &results, unsigned int module_id, request_range targets)> on_revert;

			private:
				virtual void query(patch_state &states, unsigned int module_id) override;
				virtual void apply(apply_results &results, unsigned int module_id, void *base,
					std::shared_ptr<void> lock, request_range targets) override;
				virtual void revert(revert_results &results, unsigned int module_id, request_range targets) override;
			};



			inline void patch_manager::query(patch_state &states, unsigned int module_id)
			{	on_query(states, module_id);	}

			inline void patch_manager::apply(apply_results &results, unsigned int module_id, void *base,
				std::shared_ptr<void> lock, request_range targets)
			{	on_apply(results, module_id, base, lock, targets);	}

			inline void patch_manager::revert(revert_results &results, unsigned int module_id, request_range targets)
			{	on_revert(results, module_id, targets);	}
		}
	}
}
