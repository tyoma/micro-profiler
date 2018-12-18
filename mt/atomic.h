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
		typedef int int32_t;
		typedef long long int int64_t;

		enum memory_order {
			memory_order_relaxed,
			memory_order_consume,
			memory_order_acquire,
			memory_order_release,
			memory_order_acq_rel,
			memory_order_seq_cst
		};


		template <typename T>
		class atomic
		{
		public:
			explicit atomic(T initial);

			T load(memory_order order) const volatile throw();
			void store(T value, memory_order order) volatile throw();
			bool compare_exchange_strong(T &expected, T desired, memory_order order) volatile throw();
			T fetch_add(T value, memory_order order = memory_order_seq_cst) volatile throw();

		private:
			volatile T _value;
		};


		template <typename T, size_t n = sizeof(T)>
		struct atomic_impl
		{	};

		template <typename T>
		struct atomic_impl<T, 4>
		{
			static bool compare_exchange_strong(volatile T &target, T &expected, T desired, memory_order order);
			static T fetch_add(volatile T &target, T value, memory_order order);
		};

		template <typename T>
		struct atomic_impl<T, 8>
		{
			static bool compare_exchange_strong(volatile T &target, T &expected, T desired, memory_order order);
		};


		bool compare_exchange_strong(volatile int32_t &target, int32_t &expected, int32_t desired, memory_order order);
		bool compare_exchange_strong(volatile int64_t &target, int64_t &expected, int64_t desired, memory_order order);
		int fetch_add(volatile int32_t &target, int32_t value, memory_order order);



		template <typename T>
		inline atomic<T>::atomic(T initial)
			: _value(initial)
		{	}

		template <typename T>
		inline T atomic<T>::load(memory_order /*order*/) const volatile throw()
		{	return _value;	}

		template <typename T>
		inline void atomic<T>::store(T value, memory_order /*order*/) volatile throw()
		{	_value = value;	}

		template <typename T>
		inline bool atomic<T>::compare_exchange_strong(T &expected, T desired, memory_order order) volatile throw()
		{	return atomic_impl<T, sizeof(T)>::compare_exchange_strong(_value, expected, desired, order);	}

		template <typename T>
		inline T atomic<T>::fetch_add(T value, memory_order order) volatile throw()
		{	return atomic_impl<T, sizeof(T)>::fetch_add(_value, value, order);	}


		template <typename T>
		inline bool atomic_impl<T, 4>::compare_exchange_strong(volatile T &target, T &expected, T desired,
			memory_order order)
		{
			return mt::compare_exchange_strong(reinterpret_cast<volatile int32_t &>(target),
				reinterpret_cast<int32_t &>(expected), reinterpret_cast<int32_t>(desired), order);
		}

		template <typename T>
		inline T atomic_impl<T, 4>::fetch_add(volatile T &target, T value, memory_order order)
		{	return mt::fetch_add(reinterpret_cast<volatile int32_t &>(target), static_cast<int32_t>(value), order);	}


		template <typename T>
		inline bool atomic_impl<T, 8>::compare_exchange_strong(volatile T &target, T &expected, T desired,
			memory_order order)
		{
			return mt::compare_exchange_strong(reinterpret_cast<volatile int64_t &>(target),
				reinterpret_cast<int64_t &>(expected), reinterpret_cast<int64_t>(desired), order);
		}
	}
#endif
