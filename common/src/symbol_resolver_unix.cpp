//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <common/symbol_resolver.h>

#include "elf/filemapping.h"
#include "elf/sym-elf.h"

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace
	{
		class elf_image_info : public image_info<symbol_info>
		{
		public:
			elf_image_info(const string &path);

		private:
			virtual void enumerate_functions(const symbol_callback_t &callback) const;

		private:
			shared_ptr<const symreader::mapped_region> _image;
		};



		elf_image_info::elf_image_info(const string &path)
			: _image(symreader::map_file(path.c_str()))
		{
			if (!_image->first)
				throw invalid_argument("");
		}

		void elf_image_info::enumerate_functions(const symbol_callback_t &callback) const
		{
			symreader::read_symbols(_image->first, _image->second, [&callback] (const symreader::symbol &elf_symbol) {
				symbol_info symbol = {
					elf_symbol.name,
					static_cast<unsigned>(elf_symbol.virtual_address),
					static_cast<unsigned>(elf_symbol.size)
				};

				callback(symbol);
			});
		}
	}


	shared_ptr< image_info<symbol_info> > load_image_info(const char *image_path)
	{	return shared_ptr< image_info<symbol_info> >(new elf_image_info(image_path));	}
}
