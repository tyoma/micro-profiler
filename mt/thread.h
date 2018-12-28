#pragma once

#if defined(MP_MT_GENERIC)
	#include <thread>

	namespace mt
	{
		using std::thread;
		using std::chrono::milliseconds;

		namespace this_thread
		{
			using namespace std::this_thread;
		}
	}
#else
	#include <functional>

	namespace mt
	{
		class thread
		{
		public:
			typedef unsigned int id;

		public:
			explicit thread(const std::function<void()> &f);
			~thread() throw();

			void join();
			void detach();

			id get_id() const throw();

		private:
			id _id;
			void *_thread;
		};

		typedef unsigned int milliseconds;

		namespace this_thread
		{
			thread::id get_id();
			void sleep_for(milliseconds period);
		}



		inline thread::id thread::get_id() const throw()
		{	return _id;	}
	}
#endif
