#pragma once

#include <cstddef>
#include <functional>

namespace symreader
{
	struct section
	{
		int index;
		std::ptrdiff_t file_offset;
		std::ptrdiff_t virtual_address;
		const char *name;
		unsigned int type;
		std::size_t size;
		std::size_t entry_size;
		std::size_t address_align;
	};

	struct symbol
	{
		const char *name;
		std::ptrdiff_t virtual_address;
		std::size_t size;
	};

	void read_sections(const void *image, std::size_t size, const std::function<void (const section &)> &callback);
	void read_symbols(const void *image, std::size_t size, const std::function<void (const symbol &)> &callback);
}
