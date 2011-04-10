namespace micro_profiler
{
	namespace tests
	{
		struct thread_function
		{
			virtual void operator ()() = 0;
		};

		class thread
		{
			unsigned int _threadid;
			void *_thread;

			thread(const thread &other);
			const thread &operator =(const thread &rhs);

		public:
			explicit thread(thread_function &job, bool suspended = false);
			~thread();

			unsigned int id() const;
			void resume();
		};
	}
}
