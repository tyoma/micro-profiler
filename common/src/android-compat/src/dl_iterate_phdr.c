/* libunwind - a platform-independent unwind library
   Copyright (C) 2003-2005 Hewlett-Packard Co
	Contributed by David Mosberger-Tang <davidm@hpl.hp.com>

This file is part of libunwind.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  */

#include <link.h>

#include "os-linux.h"

#ifndef IS_ELF
	/* Copied from NDK header. */
	#define IS_ELF(ehdr) ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && \
                      	(ehdr).e_ident[EI_MAG1] == ELFMAG1 && \
                      	(ehdr).e_ident[EI_MAG2] == ELFMAG2 && \
                      	(ehdr).e_ident[EI_MAG3] == ELFMAG3)

	#define ELF_W(x)	ELF32_##x
	#define Elf_W(x)	Elf32_##x
	#define elf_w(x)	_Uelf32_##x                      
#endif

/*HIDDEN*/ int
dl_iterate_phdr (int (*callback) (struct dl_phdr_info *info, size_t size, void *data),
                 void *data)
{
  int rc = 0;
  struct map_iterator mi;
  unsigned long start, end, offset, flags;

  if (maps_init (&mi, getpid()) < 0)
    return -1;

  while (maps_next (&mi, &start, &end, &offset, &flags))
    {
      Elf_W(Ehdr) *ehdr = (Elf_W(Ehdr) *) start;
      const unsigned long prot_rx = PROT_READ | PROT_EXEC;

      if (mi.path[0] != '\0' && (flags & prot_rx) == prot_rx && IS_ELF (*ehdr))
        {
          Elf_W(Phdr) *phdr = (Elf_W(Phdr) *) (start + ehdr->e_phoff);
          struct dl_phdr_info info;

          info.dlpi_addr = start;
          info.dlpi_name = mi.path;
          info.dlpi_phdr = phdr;
          info.dlpi_phnum = ehdr->e_phnum;
          rc = callback (&info, sizeof (info), data);
        }
    }

  maps_close (&mi);

  return rc;
}
