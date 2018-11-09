#include <collector/binary_translation.h>

extern "C" {
	#include <ld32.h>
}

namespace micro_profiler
{
	namespace
	{
		typedef unsigned int dword;
	}

	void move_function(byte *destination, const byte *source, size_t size)
	{
		const ptrdiff_t delta = source - destination;

		for (size_t l = 0; size; destination += l, source += l, size -= l)
		{
			switch (*source)
			{
			case 0xE9:
				*destination = *source;
				*reinterpret_cast<dword *>(destination + 1) = *reinterpret_cast<const dword *>(source + 1) + delta;
				l = 5;
				break;

			case 0xE8:
				*destination = *source;
				*reinterpret_cast<dword *>(destination + 1) = *reinterpret_cast<const dword *>(source + 1) + delta;
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
