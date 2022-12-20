//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace micro_profiler
{
	struct module
	{
		class dynamic;

		struct mapping
		{
			std::string path;
			byte *base;
			std::vector<byte_range> addresses;
		};

		struct mapping_ex
		{
			unsigned int module_id; // Persistent one-based ID of the image this mapping is for.
			std::string path;
			long_address_t base;
			std::uint32_t hash;
		};

		typedef std::function<void (const mapping &m)> mapping_callback_t;
		typedef std::pair<unsigned int /*instance_id*/, mapping_ex> mapping_instance;

		static std::shared_ptr<dynamic> load(const std::string &path);
		static std::string executable();
		static mapping locate(const void *address);
		static void enumerate_mapped(const mapping_callback_t &callback);
	};

	class module::dynamic
	{
	public:
		struct unsafe_auto_cast;

	public:
		void *find_function(const char *name) const;

	private:
		explicit dynamic(std::shared_ptr<void> handle);

	private:
		const std::shared_ptr<void> _handle;

	private:
		friend module;
	};

	struct module::dynamic::unsafe_auto_cast
	{
		template <typename T>
		operator T() const;

		void *const address;
	};



	inline module::dynamic::dynamic(std::shared_ptr<void> handle)
		: _handle(handle)
	{	}


	inline module::dynamic::unsafe_auto_cast operator /(const std::shared_ptr<module::dynamic> &library,
		const char *function_name)
	{
		module::dynamic::unsafe_auto_cast uac = {	library->find_function(function_name)	};
		return uac;
	}


	template <typename T>
	inline module::dynamic::unsafe_auto_cast::operator T() const
	{
		union {
			void *address_;
			T result;
		};

		address_ = address;
		return result;
	}
}
