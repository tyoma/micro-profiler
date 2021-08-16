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

#include "primitives.h"

#include <common/image_info.h>
#include <common/module.h>
#include <common/primitives.h>
#include <wpl/signal.h>

namespace micro_profiler
{
	namespace tables
	{
		template <typename BaseT>
		struct table : BaseT
		{
			mutable wpl::signal<void ()> invalidated;
		};


		//struct statistics : table< containers::unordered_map<unsigned int /*treadid*/,
		//	statistic_types_t<long_address_t>::map_detailed > >
		struct statistics : table<statistic_types::map_detailed>
		{
			wpl::signal<void ()> request_update;
		};


		struct threads : table< containers::unordered_map<unsigned int /*threadid*/, thread_info, knuth_hash> >
		{
			wpl::signal<void ()> request_update;
		};


		struct module_mappings : table< containers::unordered_map<unsigned int /*instance_id*/, mapped_module_identified> >
		{
			wpl::signal<void ()> updated;
		};


		struct module_info
		{
			std::string path;
			std::vector<symbol_info> symbols;
			containers::unordered_map<unsigned int /*file_id*/, std::string> files;
		};

		struct modules : table< containers::unordered_map<unsigned int /*persistent_id*/, module_info> >
		{
			wpl::signal<void (unsigned int persistent_id)> request_presence;
			mutable wpl::signal<void (unsigned int persistent_id)> ready;
		};
	}
}
