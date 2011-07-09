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

			static void sleep(unsigned int duration);
			static unsigned int current_thread_id();
		};


		template <typename T, size_t size>
		inline T *end(T (&array_ptr)[size])
		{	return array_ptr + size;	}

		template <typename T, size_t size>
		inline size_t array_size(T (&)[size])
		{	return size;	}
	}
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (AssertFailedException ^) { throw; } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
