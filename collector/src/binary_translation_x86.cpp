#include <collector/binary_translation.h>

extern "C" {
	#include <ld32.h>
}

namespace micro_profiler
{
	namespace
	{
		typedef unsigned int dword;

		bool is_target_address_32_outside(const byte *relative, const byte *range_start, size_t range_length)
		{
			relative += 4 + *reinterpret_cast<const dword *>(relative);
			return (relative < range_start) | (relative >= range_start + range_length);
		}
	}

	void move_function(byte *destination, const byte *source_base, const byte *source, size_t source_size)
	{
		const byte * const source0 = source;
		const size_t size0 = source_size;
		const ptrdiff_t delta = source_base - destination;

		for (size_t l = 0; source_size; destination += l, source += l, source_size -= l)
		{
			switch (*source)
			{
			case 0xE9:
				*destination = *source;
				*reinterpret_cast<dword *>(destination + 1) = *reinterpret_cast<const dword *>(source + 1)
					+ (is_target_address_32_outside(source + 1, source0, size0) ? delta : 0);
				l = 5;
				break;

			case 0xE8:
				*destination = *source;
				*reinterpret_cast<dword *>(destination + 1) = *reinterpret_cast<const dword *>(source + 1)
					+ (is_target_address_32_outside(source + 1, source0, size0) ? delta : 0);
				l = 5;
				break;

			default:
				l = length_disasm(const_cast<byte *>(source));
				memcpy(destination, source, l);
				break;
			}
		}
	}
}
