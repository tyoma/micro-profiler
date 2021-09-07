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

#include "module.h"
#include "primitives.h"
#include "protocol.h"
#include "range.h"

#include <patcher/interface.h>
#include <strmd/container.h>
#include <strmd/packer.h>

namespace strmd
{
	template <> struct version<micro_profiler::initialization_data> {	enum {	value = 5	};	};
	template <> struct version<micro_profiler::function_statistics> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::mapped_module_identified> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::symbol_info> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::module_info_metadata> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::thread_info> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::patch_request> {	enum {	value = 4	};	};
	template <> struct version<micro_profiler::patch_apply> {	enum {	value = 4	};	};
	template <typename KeyT> struct version< micro_profiler::function_statistics_detailed_t<KeyT> > {	enum {	value = 4	};	};
}

namespace micro_profiler
{
	typedef strmd::varint packer;

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, initialization_data &data, unsigned int ver)
	{
		archive(data.ticks_per_second);
		if (ver >= 5)
			archive(data.executable);
	}	

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, function_statistics &data, unsigned int /*ver*/)
	{
		archive(data.times_called);
		archive(data.max_reentrance);
		archive(data.inclusive_time);
		archive(data.exclusive_time);
		archive(data.max_call_time);
	}

	template <typename ArchiveT, typename AddressT>
	inline void serialize(ArchiveT &archive, function_statistics_detailed_t<AddressT> &data, unsigned int /*ver*/)
	{
		archive(static_cast<function_statistics &>(data));
		archive(data.callees);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, messages_id &data)
	{	archive(reinterpret_cast<int &>(data));	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, mapped_module_identified &data, unsigned int /*ver*/)
	{
		archive(data.instance_id);
		archive(data.persistent_id);
		archive(data.base);
		archive(data.path);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, symbol_info &data, unsigned int /*ver*/)
	{
		unsigned int id = 0;

		archive(id);
		archive(data.name);
		archive(data.rva);
		archive(data.size);
		archive(data.file_id);
		archive(data.line);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, module_info_metadata &data, unsigned int /*ver*/)
	{
		archive(data.symbols);
		archive(data.source_files);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, thread_info &data, unsigned int /*ver*/)
	{
		archive(data.native_id);
		archive(data.description);
		archive(data.start_time);
		archive(data.end_time);
		archive(data.cpu_time);
		archive(reinterpret_cast<unsigned char &>(data.complete));
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, patch_request &data, unsigned int /*ver*/)
	{
		archive(data.image_persistent_id);
		archive(data.functions_rva);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, patch_result::errors &data)
	{	archive(reinterpret_cast<int &>(data));	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, patch_apply &data, unsigned int /*ver*/)
	{
		archive(data.result);
		archive(data.id);
	}
}

namespace strmd
{
	template <typename KeyT, typename ValueT, typename HashT, typename CompT>
	struct type_traits< micro_profiler::containers::unordered_map<KeyT, ValueT, HashT, CompT> >
	{
		typedef container_type_tag category;
		typedef indexed_associative_container_reader item_reader_type;
	};

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, const void *&data)
	{	archive(reinterpret_cast<size_t &>(data));	}
}
