#include <patcher/dynamic_hooking.h>
#include <patcher/jump.h>

#include <common/memory_manager.h>
#include <common/time.h>
#include <list>
#include <mt/tls.h>
#include <test-helpers/helpers.h>

#ifdef _MSC_VER
	#include <intrin.h>
#elif !defined(__arm__)
	#include <x86intrin.h>
#endif

using namespace std;

namespace micro_profiler
{
	namespace
	{
		auto c_repetitions = 100000000;

		struct minimal_interceptor
		{
			minimal_interceptor()
				: _stack_ptr(_stack_entries)
			{	}

			static void CC_(fastcall) on_enter(minimal_interceptor* self, const void** stack_ptr,
				timestamp_t /*timestamp*/, const void* /*callee*/) _CC(fastcall)
			{	*self->_stack_ptr++ = *stack_ptr;	}

			static const void* CC_(fastcall) on_exit(minimal_interceptor* self, const void** /*stack_ptr*/,
				timestamp_t /*timestamp*/) _CC(fastcall)
			{	return *--self->_stack_ptr;	}

		private:
			const void** _stack_ptr;
			const void* _stack_entries[16];
		};

		class flat_queue : noncopyable
		{
		public:
			flat_queue(unsigned int size_power = 16)
				: _ptr(0), _mask((1 << size_power) - 1), _buffer(1 << size_power)
			{
				_buffer_start = _buffer.data();
			}

			void write_in(size_t callee, timestamp_t entered)
			{
				auto ptr = _ptr;
				auto address = _buffer_start + ptr;
				auto next = (ptr + 1) & _mask;

				address->callee = callee;
				address->timestamp = entered;
				_ptr = next;
			}

			void write_out(timestamp_t exited, timestamp_t period)
			{
				auto ptr = _ptr;
				auto address = _buffer_start + ptr;
				auto next = (ptr + 1) & _mask;

				address->callee = 0;
				address->timestamp = exited;
				_ptr = next;
			}

		private:
			struct call_record
			{
				timestamp_t timestamp;
				size_t callee;
			};

		private:
			unsigned int _ptr;
			const unsigned int _mask;
			call_record* _buffer_start;
			vector<call_record> _buffer;
		};

		class vle_queue : noncopyable
		{
		public:
			vle_queue(unsigned int size_power = 19)
				: _ptr(0), _mask((1 << size_power) - 1), _buffer(1 << size_power)
			{	_buffer_start = _buffer.data();	}

			void write_in(long_address_t callee, timestamp_t /*entered*/)
			{	_ptr = write(write_byte(_ptr, 1), callee);	}

			void write_out(timestamp_t /*entered*/, timestamp_t period)
			{	_ptr = write(write_byte(_ptr, 2), period);	}

		private:
			unsigned int write_byte(unsigned int ptr, byte value)
			{
				auto next = _mask & (ptr + 1);

				*(_buffer_start + ptr) = value;
				return next;
			}

			template <typename T>
			unsigned int write(unsigned int ptr, T value)
			{
				auto mask = _mask;
				auto buffer_start = _buffer_start;

				do
				{
					auto next = mask & (ptr + 1);
					auto nibble = static_cast<byte>(0x7F & value);

					if (value >>= 7)
						nibble &= 0x80;
					*(buffer_start + ptr) = nibble;
					ptr = next;
				} while (value);
				return ptr;
			}

		private:
			unsigned int _ptr;
			const unsigned int _mask;
			byte *_buffer_start;
			vector<byte> _buffer;
		};


		template <typename QueueT>
		struct tls_queue_manager
		{
			QueueT &get_queue()
			{
				if (auto q = _queues_tls.get())
					return *q;
				else
					return create_queue();
			}

			QueueT &get_queue_guaranteed()
			{	return *_queues_tls.get();	}

		private:
			FORCE_NOINLINE QueueT &create_queue()
			{
				auto new_q = _queues.emplace(_queues.end(), 16);

				_queues_tls.set(&*new_q);
				return *new_q;
			}

		private:
			mt::tls<QueueT> _queues_tls;
			list<QueueT> _queues;
		};

		template <typename QueueT>
		struct single_queue_manager
		{
			QueueT &get_queue()
			{	return _queue;	}

			QueueT &get_queue_guaranteed()
			{	return _queue;	}

		private:
			QueueT _queue;
		};

		template <typename QueueManagerT>
		struct queue_interceptor
		{
			queue_interceptor()
				: _stack_ptr(_stack_entries)
			{	}

			static void CC_(fastcall) on_enter(queue_interceptor*self, const void **stack_ptr,
				timestamp_t timestamp, const void *callee) _CC(fastcall)
			{
				auto ptr = self->_stack_ptr++;
				auto &q = self->_queue_manager.get_queue();

				ptr->return_address = *stack_ptr;
				ptr->entry_time = timestamp;
				q.write_in(reinterpret_cast<size_t>(callee), timestamp);
			}

			static const void *CC_(fastcall) on_exit(queue_interceptor*self, const void ** /*stack_ptr*/,
				timestamp_t timestamp) _CC(fastcall)
			{
				auto ptr = --self->_stack_ptr;
				auto &q = self->_queue_manager.get_queue_guaranteed();

				q.write_out(timestamp, timestamp - ptr->entry_time);
				return ptr->return_address;
			}

		private:
			struct stack_entry
			{
				const void *return_address;
				timestamp_t entry_time;
			};

		private:
			stack_entry *_stack_ptr;
			QueueManagerT _queue_manager;
			stack_entry _stack_entries[16];
		};

		void empty_function()
		{
		}
	}

	float measure_rdtsc(unsigned repetitions)
	{
		stopwatch sw;
		volatile long long x;

		sw();
		for (auto n = repetitions; n; n--)
			x = __rdtsc();
		return static_cast<float>(1e9 * sw() / repetitions);
	}

	template <typename InterceptorT>
	float measure_hook_overhead(unsigned repetitions)
	{
		stopwatch sw;
		InterceptorT interceptor;
		auto allocator = memory_manager(virtual_memory::granularity())
			.create_executable_allocator(const_byte_range(tests::address_cast_hack<const byte*>(&empty_function), 1), 32);
		shared_ptr<void> thunk = allocator->allocate(c_trampoline_size + c_jump_size);

		initialize_trampoline(thunk.get(), 0, &interceptor);
		jump_initialize(static_cast<byte*>(thunk.get()) + c_trampoline_size, tests::address_cast_hack<const void*>(&empty_function));

		auto f = tests::address_cast_hack<decltype(&empty_function)>(thunk.get());

		sw();
		for (auto n = repetitions; n; n--)
			f();
		return static_cast<float>(1e9 * sw() / repetitions);
	}
}

int main()
{
	using namespace micro_profiler;

	printf("rdtsc latency: %.1fns\n", measure_rdtsc(c_repetitions));
	printf("Hooked call time (minimal): %.1fns\n", measure_hook_overhead<minimal_interceptor>(c_repetitions));
	printf("Hooked call time (flat queue): %.1fns\n", measure_hook_overhead<queue_interceptor<single_queue_manager<flat_queue>>>(c_repetitions));
	printf("Hooked call time (VLE queue): %.1fns\n", measure_hook_overhead<queue_interceptor<single_queue_manager<vle_queue>>>(c_repetitions));
	printf("Hooked call time (tls, flat queue): %.1fns\n", measure_hook_overhead<queue_interceptor<tls_queue_manager<flat_queue>>>(c_repetitions));
	printf("Hooked call time (tls, VLE queue): %.1fns\n", measure_hook_overhead<queue_interceptor<tls_queue_manager<vle_queue>>>(c_repetitions));
	return 0;
}
