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
				std::function<void (patch_states &states, id_t module_id)> on_query;
				std::function<void (patch_change_results &results, id_t module_id, apply_request_range targets)> on_apply;
				std::function<void (patch_change_results &results, id_t module_id, revert_request_range targets)> on_revert;

			private:
				virtual void query(patch_states &states, id_t module_id) override;
				virtual void apply(patch_change_results &results, id_t module_id, apply_request_range targets) override;
				virtual void revert(patch_change_results &results, id_t module_id, revert_request_range targets) override;
			};



			inline void patch_manager::query(patch_states &states, id_t module_id)
			{	on_query(states, module_id);	}

			inline void patch_manager::apply(patch_change_results &results, id_t module_id, apply_request_range targets)
			{	on_apply(results, module_id, targets);	}

			inline void patch_manager::revert(patch_change_results &results, id_t module_id, revert_request_range targets)
			{	on_revert(results, module_id, targets);	}
		}
	}
}
