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

#include <string>

namespace micro_profiler
{
	enum injector_messages_id {
		request_injection,
		response_injected,
	};

	struct injection_info
	{
		std::string collector_path;
		std::string frontend_endpoint_id;
		unsigned int pid;
	};

	struct injection_response_data
	{
		unsigned int ok;
		std::string error_message;
	};



	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, injection_info &data, unsigned int /*ver*/)
	{
		archive(data.collector_path);
		archive(data.frontend_endpoint_id);
		archive(data.pid);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, injection_response_data &data, unsigned int /*ver*/)
	{
		archive(data.ok);
		archive(data.error_message);
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, injector_messages_id &data)
	{	archive(reinterpret_cast<int &>(data));	}
}

namespace strmd
{
	template <typename KeyT> struct version;
	template <> struct version< micro_profiler::injection_info > {	enum {	value = 1	};	};
	template <> struct version< micro_profiler::injection_response_data > {	enum {	value = 1	};	};
}
