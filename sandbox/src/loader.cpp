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

#include <sandbox/sandbox.h>

#include <common/module.h>
#include <ipc/endpoint_spawn.h>
#include <tuple>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		channel_ptr_t spawn::create_session(const vector<string> &arguments, channel &outbound)
		{
			typedef tuple<shared_ptr<module::dynamic>, channel_ptr_t> composite_t;

			auto m = module::platform().load(arguments.at(0));
			decltype(&ipc_spawn_server) factory = m / "ipc_spawn_server";
			channel_ptr_t session;
			vector<string> arguments2(arguments.begin() + 1, arguments.end());
			const auto composite = make_shared<composite_t>(m, session);

			factory(get<1>(*composite), arguments2, outbound);
			return channel_ptr_t(composite, get<1>(*composite).get());
		}
	}
}
