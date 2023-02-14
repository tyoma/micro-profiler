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

#include "memory.h"
#include "range.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace micro_profiler
{
	class module_platform;

	struct module
	{
		class dynamic;
		struct events;
		class lock;
		struct mapping;
		struct mapping_ex;

		typedef std::pair<unsigned int /*instance_id*/, mapping_ex> mapping_instance;

		virtual std::shared_ptr<dynamic> load(const std::string &path) = 0;
		virtual std::string executable() = 0;
		virtual mapping locate(const void *address) = 0;
		virtual std::shared_ptr<void> notify(events &consumer) = 0;

		static module &platform();
	};

	struct module::events
	{
		virtual void mapped(const mapping &mapping_) = 0;
		virtual void unmapped(void *base) = 0;
	};

	struct module::mapping
	{
		std::string path;
		byte *base;
		std::vector<mapped_region> regions;
	};

	struct module::mapping_ex
	{
		unsigned int module_id; // Persistent one-based ID of the image this mapping is for.
		std::string path;
		long_address_t base;
		std::uint32_t hash;
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
		friend module_platform;
	};

	class module::lock
	{
	public:
		lock(const void *base, const std::string &path);
		lock(lock &&other);
		~lock();

		operator bool() const;

	private:
		void operator =(lock &&rhs);
		void operator ==(const lock &rhs);

	private:
		void *_handle;
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


	inline module::lock::lock(lock &&other)
		: _handle(other._handle)
	{	other._handle = nullptr;	}

	inline module::lock::operator bool() const
	{	return !!_handle;	}


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
