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

#include "frontend_ui.h"
#include "serialization_context.h"

#include <common/noncopyable.h>
#include <common/protocol.h>
#include <common/unordered_map.h>
#include <functional>
#include <ipc/client_session.h>
#include <list>

namespace micro_profiler
{
	namespace tables
	{
		struct module_mappings;
		struct modules;
		struct patches;
	}

	class symbol_resolver;
	class threads_model;

	class frontend : public ipc::client_session, noncopyable
	{
	public:
		frontend(ipc::channel &outbound);
		~frontend();

	public:
		std::function<void (const frontend_ui_context &ui_context)> initialized;

	private:
		typedef std::list< std::shared_ptr<void> > requests_t;

	private:
		// ipc::channel methods
		virtual void disconnect() throw() override;

		void init_patcher();
		void apply(unsigned int persistent_id, range<const unsigned int, size_t> rva);
		void revert(unsigned int persistent_id, range<const unsigned int, size_t> rva);

		void request_full_update();

		requests_t::iterator new_request_handle();

	private:
		initialization_data _process_info;
		const std::shared_ptr<tables::statistics> _statistics;
		const std::shared_ptr<tables::modules> _modules;
		const std::shared_ptr<tables::module_mappings> _mappings;
		const std::shared_ptr<tables::patches> _patches;
		std::shared_ptr<threads_model> _threads;
		scontext::wire _serialization_context;
		bool _initialized;

		containers::unordered_map< unsigned int /*persistent_id*/, std::shared_ptr<void> > _module_requests;
		std::list< std::shared_ptr<void> > _requests;
		std::shared_ptr<void> _update_request;

		// request_apply_patches buffers
		patch_request _patch_request_payload;
		response_patched_data _patched_buffer;

		// request_revert_patches buffers
		response_reverted_data _reverted_buffer;
	};
}
