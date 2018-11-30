#pragma once

#if defined(MP_MT_GENERIC)
	#include <atomic>

	namespace mt
	{
		using std::atomic;
		using std::memory_order_relaxed;
		using std::memory_order_consume;
		using std::memory_order_acquire;
		using std::memory_order_release;
		using std::memory_order_acq_rel;
		using std::memory_order_seq_cst;
	}
#else
	namespace mt
	{
		enum memory_order {
			memory_order_relaxed,
			memory_order_consume,
			memory_order_acquire,
			memory_order_release,
			memory_order_acq_rel,
			memory_order_seq_cst
		};


		template <typename T, size_t n = sizeof(T)>
		class atomic
		{
		public:
			explicit atomic(T initial);

			T load(memory_order order) const volatile throw();
			void store(T value, memory_order order) volatile throw();
			bool compare_exchange_strong(T &expected, T desired, memory_order order) volatile throw();

		private:
			volatile T _value;
		};


		template <typename T, size_t n = sizeof(T)>
		struct atomic_cmpxchg
		{	};

		template <typename T>
		struct atomic_cmpxchg<T, 4>
		{	static bool compare_exchange_strong(volatile T &target, T &expected, T desired, memory_order order);	};

		template <typename T>
		struct atomic_cmpxchg<T, 8>
		{	static bool compare_exchange_strong(volatile T &target, T &expected, T desired, memory_order order);	};



		template <typename T, size_t n>
		inline atomic<T, n>::atomic(T initial)
			: _value(initial)
		{	}

		template <typename T, size_t n>
		inline T atomic<T, n>::load(memory_order /*order*/) const volatile throw()
		{	return _value;	}

		template <typename T, size_t n>
		inline void atomic<T, n>::store(T value, memory_order /*order*/) volatile throw()
		{	_value = value;	}

		template <typename T, size_t n>
		inline bool atomic<T, n>::compare_exchange_strong(T &expected, T desired, memory_order order) volatile throw()
		{	return atomic_cmpxchg<T, n>::compare_exchange_strong(_value, expected, desired, order);	}


		bool compare_exchange_strong(long volatile &target, long &expected, long desired, memory_order order);
		bool compare_exchange_strong(long long volatile &target, long long &expected, long long desired,
			memory_order order);

		template <typename T>
		inline bool atomic_cmpxchg<T, 4>::compare_exchange_strong(volatile T &target, T &expected, T desired,
			memory_order order)
		{
			return mt::compare_exchange_strong(reinterpret_cast<volatile long &>(target), reinterpret_cast<long &>(expected),
				reinterpret_cast<long>(desired), order);
		}

		template <typename T>
		inline bool atomic_cmpxchg<T, 8>::compare_exchange_strong(volatile T &target, T &expected, T desired,
			memory_order order)
		{
			return mt::compare_exchange_strong(reinterpret_cast<volatile long long &>(target),
				reinterpret_cast<long long &>(expected), reinterpret_cast<long long>(desired), order);
		}
	}
#endif
