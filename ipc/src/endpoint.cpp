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

#include <ipc/endpoint_com.h>
#include <ipc/endpoint_sockets.h>

#include <algorithm>
#include <functional>
#include <string>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace ipc
	{
		typedef function<shared_ptr<void> (const char *typed_endpoint_id, const shared_ptr<server> &factory)>
			server_endpoint_factory_t;

		struct constructors
		{
			string protocol;
			server_endpoint_factory_t server_constructor;
		};

		const constructors c_constructors[] = {
#ifdef _WIN32
			{ "com", &com::run_server },
#endif
			{ "sockets", &sockets::run_server },
		};


		initialization_failed::initialization_failed()
			: runtime_error("")
		{	}

		protocol_not_supported::protocol_not_supported(const char *message)
			: invalid_argument(message)
		{	}


		connection_refused::connection_refused(const char *message)
			: runtime_error(message)
		{	}


		shared_ptr<void> run_server(const char *typed_endpoint_id, const shared_ptr<server> &factory)
		{
			if (!typed_endpoint_id)
				throw invalid_argument("");
			const string id = typed_endpoint_id;
			const size_t delim = id.find('|');
			if (delim == string::npos)
				throw invalid_argument(typed_endpoint_id);
			const string protocol = id.substr(0, delim);
			const string endpoint_id = id.substr(delim + 1);

			for (size_t i = 0, count = sizeof(c_constructors) / sizeof(c_constructors[0]); i != count; ++i)
			{
				if (protocol == c_constructors[i].protocol)
					return c_constructors[i].server_constructor(endpoint_id.c_str(), factory);
			}
			throw protocol_not_supported(typed_endpoint_id);
		}
	}
}
