#pragma once

#include <common/module.h>
#include <common/noncopyable.h>
#include <common/primitives.h>
#include <common/protocol.h>
#include <functional>
#include <ipc/endpoint.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			typedef function_statistics_detailed_t<unsigned int> function_statistics_detailed;
			typedef statistics_map_detailed_t<unsigned int> statistics_map_detailed;

			class frontend_state : noncopyable, public std::enable_shared_from_this<frontend_state>
			{
			public:
				explicit frontend_state(const std::shared_ptr<void> &ownee = std::shared_ptr<void>());

				std::shared_ptr<ipc::channel> create();

			public:
				std::function<void ()> constructed;
				std::function<void ()> destroyed;

				std::function<void (const initialization_data &id)> initialized;
				std::function<void (const loaded_modules &m)> modules_loaded;
				std::function<void (const statistics_map_detailed &u)> updated;
				std::function<void (const unloaded_modules &m)> modules_unloaded;
				std::function<void (unsigned id, const module_info_metadata &md)> metadata_received;

			private:
				std::shared_ptr<void> _ownee;
			};



			template <typename ArchiveT>
			inline void serialize(ArchiveT &a, frontend_state &state)
			{
				unsigned metadata_id;
				commands c;
				initialization_data id;
				loaded_modules lm;
				statistics_map_detailed u;
				unloaded_modules um;
				module_info_metadata md;

				a(c);
				switch (c)
				{
				case init:
					if (state.initialized)
						a(id), state.initialized(id);
					break;

				case modules_loaded:
					if (state.modules_loaded)
						a(lm), state.modules_loaded(lm);
					break;

				case update_statistics:
					if (state.updated)
						a(u), state.updated(u);
					break;

				case modules_unloaded:
					if (state.modules_unloaded)
						a(um), state.modules_unloaded(um);
					break;

				case module_metadata:
					if (state.metadata_received)
						a(metadata_id), a(md), state.metadata_received(metadata_id, md);
					break;

				default:
					break;
				}
			}


			inline frontend_state::frontend_state(const std::shared_ptr<void> &ownee)
				: _ownee(ownee)
			{	}
		}
	}
}
