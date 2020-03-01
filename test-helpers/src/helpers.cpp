#include <test-helpers/helpers.h>

#include <common/memory.h>
#include <common/time.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			byte rand()
			{	return static_cast<byte>(::rand());	}
		}

		guid_t generate_id()
		{
			static bool seed_initialized = false;

			if (!seed_initialized)
				srand(static_cast<unsigned int>(clock())), seed_initialized = true;

			guid_t id;

			generate(id.values, array_end(id.values), &rand);
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
			const byte *b = reinterpret_cast<const byte *>(buffer_);

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
}
