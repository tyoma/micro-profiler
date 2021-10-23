//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#pragma once

#include "range.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace micro_profiler
{
	struct mapped_module_ex;
	typedef std::pair<unsigned int /*instance_id*/, mapped_module_ex> mapped_module_identified;

	struct mapping_less
	{
		bool operator ()(const mapped_module_identified &lhs, const mapped_module_identified &rhs) const;
		bool operator ()(const mapped_module_identified &lhs, long_address_t rhs) const;
		bool operator ()(long_address_t lhs, const mapped_module_identified &rhs) const;
	};

	struct mapped_module
	{
		std::string path;
		byte *base;
		std::vector<byte_range> addresses;
	};

	struct mapped_module_ex
	{
		static mapped_module_identified from(unsigned int instance_id_, unsigned int persistent_id_,
			const mapped_module &mm);

		unsigned int persistent_id; // Persistent one-based ID of the image this mapping is for.
		std::string path;
		long_address_t base;
	};

	typedef std::function<void (const mapped_module &module)> module_callback_t;

	std::shared_ptr<void> load_library(const std::string &path);
	std::string get_current_executable();
	mapped_module get_module_info(const void *address);
	void enumerate_process_modules(const module_callback_t &callback);



	inline bool mapping_less::operator ()(const mapped_module_identified &lhs, const mapped_module_identified &rhs) const
	{	return lhs.second.base < rhs.second.base;	}

	inline bool mapping_less::operator ()(const mapped_module_identified &lhs, long_address_t rhs) const
	{	return lhs.second.base < rhs;	}

	inline bool mapping_less::operator ()(long_address_t lhs, const mapped_module_identified &rhs) const
	{	return lhs < rhs.second.base;	}

	inline mapped_module_identified mapped_module_ex::from(unsigned int instance_id_,
		unsigned int persistent_id_, const mapped_module &mm)
	{
		mapped_module_ex mmi = {
			persistent_id_,
			mm.path,
			reinterpret_cast<size_t>(mm.base),
		};

		return mapped_module_identified(instance_id_, mmi);
	}
}
