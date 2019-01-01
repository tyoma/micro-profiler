#include <test-helpers/helpers.h>

#include <common/memory.h>
#include <common/time.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		guid_t generate_id()
		{
			static bool seed_initialized = false;

			if (!seed_initialized)
				srand(static_cast<unsigned int>(clock())), seed_initialized = true;

			guid_t id;

			generate(id.values, array_end(id.values), rand);
			return id;
		}


		vector_adapter::vector_adapter()
			: ptr(0)
		{	}

		vector_adapter::vector_adapter(const_byte_range from)
			: ptr(0), buffer(from.begin(), from.end())
		{	}

		void vector_adapter::write(const void *buffer_, size_t size)
		{
			const unsigned char *b = reinterpret_cast<const unsigned char *>(buffer_);

			buffer.insert(buffer.end(), b, b + size);
		}

		void vector_adapter::read(void *buffer_, size_t size)
		{
			assert_is_true(size <= buffer.size() - ptr);
			mem_copy(buffer_, &buffer[ptr], size);
			ptr += size;
		}

		void vector_adapter::rewind(size_t pos)
		{	ptr = pos;	}

		size_t vector_adapter::end_position() const
		{	return buffer.size();	}
	}

	bool operator <(const function_statistics &lhs, const function_statistics &rhs)
	{
		return lhs.times_called < rhs.times_called ? true : lhs.times_called > rhs.times_called ? false :
			lhs.max_reentrance < rhs.max_reentrance ? true : lhs.max_reentrance > rhs.max_reentrance ? false :
			lhs.inclusive_time < rhs.inclusive_time ? true : lhs.inclusive_time > rhs.inclusive_time ? false :
			lhs.exclusive_time < rhs.exclusive_time ? true : lhs.exclusive_time > rhs.exclusive_time ? false :
			lhs.max_call_time < rhs.max_call_time;
	}

	bool operator ==(const function_statistics &lhs, const function_statistics &rhs)
	{	return !(lhs < rhs) && !(rhs < lhs);	}
}
