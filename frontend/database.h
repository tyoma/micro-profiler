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

#include "constructors.h"
#include "primitives.h"

#include <common/image_info.h>
#include <common/module.h>
#include <common/noncopyable.h>
#include <common/protocol.h>
#include <sdb/table.h>

namespace micro_profiler
{
	typedef sdb::table< call_statistics, auto_increment_constructor<call_statistics> > calls_statistics_table;

	namespace tables
	{
		template <typename T>
		struct record : identity, T
		{	};


		struct statistics : calls_statistics_table
		{
			std::function<void ()> request_update;
		};


		typedef record<thread_info> thread;
		typedef sdb::table<thread> threads;


		typedef record<module::mapping_ex> module_mapping;
		typedef sdb::table<module_mapping> module_mappings;


		typedef record<module_info_metadata> module;
		struct modules : sdb::table<module>
		{
			typedef std::shared_ptr<void> handle_t;

			typedef std::function<void (const module_info_metadata &metadata)> metadata_ready_cb;
			std::function<void (handle_t &request, unsigned int module_id, const metadata_ready_cb &ready)>
				request_presence;

			mutable wpl::signal<void ()> invalidate;
		};


		struct symbol_info : micro_profiler::symbol_info
		{
			id_t module_id;
		};


		struct source_file : identity
		{
			id_t module_id;
			std::string path;
		};


		struct patches : sdb::table<patch>
		{
			std::function<void (unsigned int module_id, range<const unsigned int, size_t> rva)> apply;
			std::function<void (unsigned int module_id, range<const unsigned int, size_t> rva)> revert;
		};

		struct cached_patch
		{
			id_t scope_id;
			id_t module_id;
			unsigned int rva;
		};
	}

	struct profiling_session : noncopyable
	{
		initialization_data process_info;
		tables::statistics statistics;
		tables::module_mappings mappings;
		tables::modules modules;
		tables::patches patches;
		tables::threads threads;
	};



	inline std::string get_title(std::shared_ptr<const profiling_session> session)
	{	return session->process_info.executable;	}

	inline std::shared_ptr<tables::statistics> statistics(std::shared_ptr<profiling_session> session)
	{	return std::shared_ptr<tables::statistics>(session, &session->statistics);	}

	inline std::shared_ptr<tables::module_mappings> mappings(std::shared_ptr<profiling_session> session)
	{	return std::shared_ptr<tables::module_mappings>(session, &session->mappings);	}

	inline std::shared_ptr<tables::modules> modules(std::shared_ptr<profiling_session> session)
	{	return std::shared_ptr<tables::modules>(session, &session->modules);	}

	inline std::shared_ptr<tables::threads> threads(std::shared_ptr<profiling_session> session)
	{	return std::shared_ptr<tables::threads>(session, &session->threads);	}

	inline std::shared_ptr<tables::patches> patches(std::shared_ptr<profiling_session> session)
	{	return std::shared_ptr<tables::patches>(session, &session->patches);	}
}
