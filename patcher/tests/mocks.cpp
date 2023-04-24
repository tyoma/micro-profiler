#include "mocks.h"

#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			void CC_(fastcall) trace_events::on_enter(trace_events *self, const void **stack_ptr,
				timestamp_t timestamp, const void *callee)
			{
				call_record call = { timestamp, callee };

				self->return_stack.push_back(make_pair(*stack_ptr, stack_ptr));
				self->call_log.push_back(call);
				self->enter_stack_addresses.push_back(stack_ptr);
			}

			const void *CC_(fastcall) trace_events::on_exit(trace_events *self, const void **stack_ptr,
				timestamp_t timestamp)
			{
				call_record call = { timestamp, 0 };
				const void *return_address = self->return_stack.back().first;

				self->return_stack.pop_back();
				self->call_log.push_back(call);
				self->exit_stack_addresses.push_back(stack_ptr);
				return return_address;
			}


			mapping_access::mapping_access()
				: subscription(nullptr)
			{	}

			shared_ptr<module::mapping> mapping_access::lock_mapping(id_t mapping_id)
			{	return on_lock_mapping(mapping_id);	}

			shared_ptr<void> mapping_access::notify(events &events_)
			{
				assert_null(subscription);
				subscription = &events_;
				return shared_ptr<void>(subscription, [this] (void *) {	subscription = nullptr;	});
			}


			vector<memory_manager::lock_info> memory_manager::locks() const
			{	return vector<lock_info>(_locks.begin(), _locks.end());	}

			shared_ptr<executable_memory_allocator> memory_manager::create_executable_allocator(const_byte_range,
				ptrdiff_t)
			{
				allocators.push_back(make_shared<executable_memory_allocator>());
				return allocators.back();
			}

			shared_ptr<void> memory_manager::scoped_protect(byte_range region, int scoped_protection,
				int released_protection)
			{
				auto i = _locks.insert(_locks.end(), make_tuple(region, scoped_protection, released_protection));

				return shared_ptr<void>(&*i, [this, i] (void *) {	_locks.erase(i);	});
			}


			patch::patch(function<void (void *target, int act)> on_patch_action, void *target)
				: _on_patch_action(on_patch_action), _target(target)
			{	_on_patch_action(_target, 0);	}

			patch::~patch()
			{	_on_patch_action(_target, 3);	}

			bool patch::activate()
			{	return _on_patch_action(_target, 1), true;	}

			bool patch::revert()
			{	return _on_patch_action(_target, 2), true;	}
		}
	}
}
