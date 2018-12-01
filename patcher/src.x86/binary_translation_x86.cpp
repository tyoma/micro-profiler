#include <patcher/binary_translation.h>

#include <common/memory.h>
#include <stddef.h>

extern "C" {
	#include <ld32.h>
}

namespace micro_profiler
{
	namespace
	{
		typedef unsigned int dword;
		typedef char sbyte;
		typedef int sdword;

		template <typename DispT>
		bool is_target_inside(const byte *disp, const_byte_range source)
		{	return source.inside(disp + sizeof(DispT) + *reinterpret_cast<const DispT *>(disp));	}
	}

	inconsistent_function_range_exception::inconsistent_function_range_exception(const char *message)
		: runtime_error(message)
	{	}


	size_t calculate_function_length(const_byte_range source, size_t min_length)
	{
		size_t result = 0, l = 0;
		const byte *ptr = source.begin();

		for (bool end = false; !end; end = min_length <= l, min_length -= l, result += l, ptr += l)
			l = length_disasm((void *)ptr);
		return result;
	}

	void move_function(byte *destination, const_byte_range source_)
	{
		const byte *source = source_.begin();
		size_t size = source_.length();
		const ptrdiff_t delta = source - destination;

		for (size_t l = 0; size; destination += l, source += l, size -= l)
		{
			l = length_disasm(const_cast<byte *>(source));

			switch (*source)
			{
			case 0x70:
			case 0x71:
			case 0x72:
			case 0x73:
			case 0x74:
			case 0x75:
			case 0x76:
			case 0x77:
			case 0x78:
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7e:
			case 0x7f:
			case 0xEB:
				if (!is_target_inside<sbyte>(source + 1, source_))
					throw inconsistent_function_range_exception("Short relative jump outside the copied range is met!");
				*destination = *source, *(destination + 1) = *(source + 1);
				continue;

			case 0xCC:
				throw inconsistent_function_range_exception("Debug interrupt met!");

			case 0xE9:
			case 0xE8:
				*destination = *source;
				*reinterpret_cast<dword *>(destination + 1) = static_cast<dword>(*reinterpret_cast<const dword *>(source + 1)
					+ (!is_target_inside<sdword>(source + 1, source_) ? delta : 0));
				continue;

			case 0x0F:
				switch (*(source + 1))
				{
				case 0x80:
				case 0x81:
				case 0x82:
				case 0x83:
				case 0x84:
				case 0x85:
				case 0x86:
				case 0x87:
				case 0x88:
				case 0x89:
				case 0x8a:
				case 0x8b:
				case 0x8c:
				case 0x8d:
				case 0x8e:
				case 0x8f:
					*destination = *source,  *(destination + 1) = *(source + 1);
					*reinterpret_cast<dword *>(destination + 2) = static_cast<dword>(*reinterpret_cast<const dword *>(source + 2)
						+ (!is_target_inside<sdword>(source + 2, source_) ? delta : 0));
					continue;
				}

			default:
				mem_copy(destination, source, l);
				continue;
			}
		}
	}
}
