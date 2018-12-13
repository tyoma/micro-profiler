#include <patcher/binary_translation.h>

#include <common/memory.h>
#include <stddef.h>

extern "C" {
	#include <ld32.h>
}

using namespace std;

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

		template <typename ByteT>
		class instruction_iterator
		{
		public:
			instruction_iterator(range<ByteT, size_t> body)
				: _ptr(body.begin()), _remaining_length(body.length()), _current_length(0)
			{	}

			ByteT *ptr() const
			{	return _ptr;	}

			byte length() const
			{	return _current_length;	}

			bool fetch()
			{
				if (_remaining_length <= _current_length)
					return false;
				_ptr += _current_length;
				_remaining_length -= _current_length;
				_current_length = static_cast<byte>(length_disasm((void *)_ptr));
				if (_current_length > _remaining_length)
					throw inconsistent_function_range_exception("Attempt to read past the function body!"); // TODO: test!
				return true;
			}

		private:
			ByteT *_ptr;
			size_t _remaining_length;
			byte _current_length;
		};
	}

	inconsistent_function_range_exception::inconsistent_function_range_exception(const char *message)
		: runtime_error(message)
	{	}


	size_t calculate_fragment_length(const_byte_range source, size_t min_length)
	{
		ptrdiff_t remainder = min_length;
		size_t actual_length = 0;

		for (instruction_iterator<const byte> i(source); remainder > 0 && i.fetch(); remainder -= i.length())
			actual_length += i.length();
		return actual_length;
	}

	void move_function(byte *destination, const_byte_range source)
	{
		const ptrdiff_t delta = source.begin() - destination;

		for (instruction_iterator<const byte> i(source); i.fetch(); destination += i.length())
		{
			switch (*i.ptr())
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
				if (!is_target_inside<sbyte>(i.ptr() + 1, source))
					throw inconsistent_function_range_exception("Short relative jump outside the copied range is met!");
				break;

			case 0xCC:
				throw inconsistent_function_range_exception("Debug interrupt met!");

			case 0xE9:
			case 0xE8:
				if (!is_target_inside<sdword>(i.ptr() + 1, source))
				{
					*destination = *i.ptr();
					*reinterpret_cast<dword *>(destination + 1)
						= static_cast<dword>(*reinterpret_cast<const dword *>(i.ptr() + 1)) + delta;
					continue;
				}
				break;

			case 0x0F:
				switch (*(i.ptr() + 1))
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
					if (!is_target_inside<sdword>(i.ptr() + 2, source))
					{
						*destination = *i.ptr();
						*(destination + 1) = *(i.ptr() + 1);
						*reinterpret_cast<dword *>(destination + 2)
							= static_cast<dword>(*reinterpret_cast<const dword *>(i.ptr() + 2)) + delta;
						continue;
					}
					break;
				}
			}
			mem_copy(destination, i.ptr(), i.length());
		}
	}

	void offset_displaced_references(revert_buffer &rbuffer, byte_range source, const_byte_range displaced_region,
		const byte *displaced_to)
	{
		const ptrdiff_t delta = displaced_to - displaced_region.begin();

		for (instruction_iterator<byte> i(source); i.fetch(); )
			switch (*i.ptr())
			{
			case 0xE9:
				if (is_target_inside<sdword>(i.ptr() + 1, displaced_region))
				{
					rbuffer.push_back(revert_entry<>(i.ptr() + 1, 4));
					*reinterpret_cast<dword *>(i.ptr() + 1) += delta;
				}
				break;
			}
	}
}
