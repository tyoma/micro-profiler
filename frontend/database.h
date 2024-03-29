//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/auto_increment.h>
#include <common/image_info.h>
#include <common/module.h>
#include <common/noncopyable.h>
#include <common/protocol.h>
#include <common/smart_ptr.h>
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
			std::function<void (handle_t &request, id_t module_id, const metadata_ready_cb &ready)>
				request_presence;

			mutable wpl::signal<void ()> invalidate;
		};


		struct symbol_info : micro_profiler::symbol_info
		{
			id_t module_id;
		};
		typedef sdb::table<symbol_info> symbols;


		struct source_file : identity
		{
			id_t module_id;
			std::string path;
		};
		typedef sdb::table<source_file> source_files;


		struct patches : sdb::table<patch_state_ex>
		{
			typedef std::pair<unsigned int, unsigned int> patch_def;

			std::function<void (id_t module_id, range<const patch_def, size_t> rva)> apply;
			std::function<void (id_t module_id, range<const unsigned int, size_t> rva)> revert;
		};

		struct cached_patch
		{
			id_t scope_id;
			id_t module_id;
			unsigned int rva, size;
		};
	}

	struct profiling_session : noncopyable
	{
		initialization_data process_info;
		tables::statistics statistics;
		tables::module_mappings mappings;
		tables::modules modules;
		tables::symbols symbols;
		tables::source_files source_files;
		tables::patches patches;
		tables::threads threads;
	};



	inline std::string get_title(std::shared_ptr<const profiling_session> session)
	{	return session->process_info.executable;	}

	inline std::shared_ptr<tables::statistics> statistics(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->statistics);	}

	inline std::shared_ptr<tables::module_mappings> mappings(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->mappings);	}

	inline std::shared_ptr<tables::modules> modules(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->modules);	}

	inline std::shared_ptr<tables::symbols> symbols(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->symbols);	}

	inline std::shared_ptr<tables::source_files> source_files(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->source_files);	}

	inline std::shared_ptr<tables::threads> threads(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->threads);	}

	inline std::shared_ptr<tables::patches> patches(std::shared_ptr<profiling_session> session)
	{	return make_shared_aspect(session, &session->patches);	}
}
