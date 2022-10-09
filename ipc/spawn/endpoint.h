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

#include "../endpoint_spawn.h"

#include <mt/thread.h>
#include <stdio.h>

namespace micro_profiler
{
	namespace ipc
	{
		namespace spawn
		{
			class client_session : public channel
			{
			public:
				client_session(const std::string &spawned_path, const std::vector<std::string> &arguments, channel &inbound);
				~client_session();

				static std::pair<std::shared_ptr<FILE> /*outbound*/, std::shared_ptr<FILE> /*inbound*/> spawn(
					const std::string &spawned_path, const std::vector<std::string> &arguments);

			private:
				virtual void disconnect() throw() override;
				virtual void message(const_byte_range payload) override;

			private:
				std::unique_ptr<mt::thread> _thread;
				std::shared_ptr<FILE> _outbound;
			};
		}
	}
}
