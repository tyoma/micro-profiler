#pragma once

#if defined(MP_MT_GENERIC)
	#include <mutex>

	namespace mt
	{
		using mutex = std::mutex;
		using recursive_mutex = std::recursive_mutex;
		using std::lock_guard;
		using std::unique_lock;
	}
#else
	#include <common/noncopyable.h>

	namespace mt
	{
		class mutex : micro_profiler::noncopyable
		{
		public:
			mutex();
			~mutex();

			void lock();
			void unlock();

		private:
			unsigned char _mtx_buffer[6 * sizeof(void*)];
		};

		typedef mutex recursive_mutex;


		template <typename MutexT>
		class lock_guard : micro_profiler::noncopyable
		{
		public:
			lock_guard(MutexT &mtx);
			~lock_guard();

		private:
			MutexT &_mutex;
		};


		template <typename MutexT>
		class unique_lock : micro_profiler::noncopyable
		{
		public:
			unique_lock(MutexT &mtx);
			~unique_lock();

			void lock();
			void unlock();

		private:
			MutexT &_mutex;
			bool _locked;
		};



		template <typename MutexT>
		inline lock_guard<MutexT>::lock_guard(MutexT &mtx)
			: _mutex(mtx)
		{	_mutex.lock();	}

		template <typename MutexT>
		inline lock_guard<MutexT>::~lock_guard()
		{	_mutex.unlock();	}


		template <typename MutexT>
		inline unique_lock<MutexT>::unique_lock(MutexT &mtx)
			: _mutex(mtx), _locked(true)
		{	_mutex.lock();	}

		template <typename MutexT>
		inline unique_lock<MutexT>::~unique_lock()
		{	unlock();	}

		template <typename MutexT>
		inline void unique_lock<MutexT>::lock()
		{
			if (!_locked)
				_mutex.lock(), _locked = true;
		}

		template <typename MutexT>
		inline void unique_lock<MutexT>::unlock()
		{
			if (_locked)
				_mutex.unlock(), _locked = false;
		}
	}
#endif
