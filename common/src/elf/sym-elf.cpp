#include "sym-elf.h"

#include <cstdint>
#include <elf.h>
#include <string.h>

using namespace std;

namespace symreader
{
	namespace
	{
		enum bitness { _unsupported, _32bit, _64bit };

		bitness get_bitness(const void *image)
		{
			if (static_cast<const uint8_t *>(image)[EI_CLASS] == ELFCLASS32)
				return _32bit;
			else if (static_cast<const uint8_t *>(image)[EI_CLASS] == ELFCLASS64)
				return _64bit;
			return _unsupported;
		}

		template <typename ElfHeaderT, typename SectionHeaderT, typename CallbackT>
		void read_sections(const void *image, size_t size, const CallbackT &callback)
		{
			const ElfHeaderT *ehdr = static_cast<const ElfHeaderT *>(image);
			const SectionHeaderT *shdrs = (const SectionHeaderT *)((const uint8_t *)image + ehdr->e_shoff);
			const SectionHeaderT *strhdr = &shdrs[ehdr->e_shstrndx];
			const char *strtab = static_cast<const char *>(image) + strhdr->sh_offset;

			for (int i = 0; i < ehdr->e_shnum; ++i)
			{
				section s;

				s.index= i;
				s.name = strtab + shdrs[i].sh_name;
				s.type = shdrs[i].sh_type;
				s.virtual_address = static_cast<ptrdiff_t>(shdrs[i].sh_addr);
				s.file_offset = static_cast<ptrdiff_t>(shdrs[i].sh_offset);
				s.size = static_cast<size_t>(shdrs[i].sh_size);
				s.entry_size = static_cast<size_t>(shdrs[i].sh_entsize);
				s.address_align = static_cast<size_t>(shdrs[i].sh_addralign);
				callback(s);
			}
		}


		template <typename SymbolEntryT, typename CallbackT>
		void read_symbols(const void *image, unsigned int code_section_index, const section &symbols, const char *names,
			const CallbackT &callback)
		{
			const size_t total_syms = symbols.size / sizeof(SymbolEntryT);
			const SymbolEntryT *syms_data = (const SymbolEntryT *)((const uint8_t *)image + symbols.file_offset);

			for (size_t i = 0; i < total_syms; ++i)
			{
				symbol s = { };
				const SymbolEntryT &sd = syms_data[i];
				const unsigned type = ELF32_ST_TYPE(sd.st_info);

				if (type != STT_FUNC)
					continue;
				if (sd.st_shndx != code_section_index || !sd.st_size)
					continue;
				s.name = names + sd.st_name;
				s.size = static_cast<size_t>(sd.st_size);
				s.virtual_address = static_cast<ptrdiff_t>(sd.st_value);
				callback(s);
			}
		}
	}

	void read_sections(const void *image, size_t size, const std::function<void (const section &)> &callback)
	{
		if (_32bit == get_bitness(image))
			read_sections<Elf32_Ehdr, Elf32_Shdr>(image, size, callback);
		else if (_64bit == get_bitness(image))
			read_sections<Elf64_Ehdr, Elf64_Shdr>(image, size, callback);
	}

	void read_symbols(const void *image, size_t size, const std::function<void (const symbol &)> &callback)
	{
		unsigned code_section_index = 0;
		section symtab = { }, dynsym = { };
		const char *strtab = 0, *dynstr = 0;

		read_sections(image, size, [&] (const section &s) {
			if (!strcmp(s.name, ".text"))
				code_section_index = s.index;
			switch (s.type)
			{
			case SHT_STRTAB:
				if (!strcmp(s.name, ".strtab"))
					strtab = static_cast<const char *>(image) + s.file_offset;
				else if (!strcmp(s.name, ".dynstr"))
					dynstr = static_cast<const char *>(image) + s.file_offset;
				break;

			case SHT_SYMTAB:
				symtab = s;
				break;

			case SHT_DYNSYM:
				dynsym = s;
				break;
			}
		});

		switch (get_bitness(image))
		{
		case _32bit:
			if (strtab)
				read_symbols<Elf32_Sym>(image, code_section_index, symtab, strtab, callback);
			if (dynstr)
				read_symbols<Elf32_Sym>(image, code_section_index, dynsym, dynstr, callback);
			break;

		case _64bit:
			if (strtab)
				read_symbols<Elf64_Sym>(image, code_section_index, symtab, strtab, callback);
			if (dynstr)
				read_symbols<Elf64_Sym>(image, code_section_index, dynsym, dynstr, callback);
			break;
		}
	}
}
