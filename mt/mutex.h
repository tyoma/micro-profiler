#pragma once

#if defined(MP_MT_GENERIC)
	#include <mutex>

	namespace mt
	{
		using std::mutex;
		using std::lock_guard;
	}
#else
	#include <wpl/base/concepts.h>

	namespace mt
	{
		class mutex : wpl::noncopyable
		{
		public:
			mutex();
			~mutex();

			void lock();
			void unlock();

		private:
			unsigned char _mtx_buffer[6 * sizeof(void*)];
		};
	
	
		template <typename MutexT>
		class lock_guard : wpl::noncopyable
		{
		public:
			lock_guard(MutexT &mtx);
			~lock_guard();

		private:
			MutexT &_mutex;
		};



		template <typename MutexT>
		inline lock_guard<MutexT>::lock_guard(MutexT &mtx)
			: _mutex(mtx)
		{	_mutex.lock();	}

		template <typename MutexT>
		inline lock_guard<MutexT>::~lock_guard()
		{	_mutex.unlock();	}
	}
#endif
