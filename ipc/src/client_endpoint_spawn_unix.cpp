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

#include <ipc/spawn/endpoint.h>

#include <memory>
#include <unistd.h>

using namespace std;

namespace micro_profiler
{
	namespace ipc
	{
		namespace spawn
		{
			namespace
			{
				shared_ptr<FILE> from_fd(int fd, const char *mode)
				{
					if (auto stream = fdopen(fd, mode))
						return shared_ptr<FILE>(stream, &fclose);
					::close(fd);
					throw runtime_error("crt stream error");
				}
			}

			pair< shared_ptr<FILE>, shared_ptr<FILE> > client_session::spawn(const string &spawned_path,
				const vector<string> &arguments)
			{
				// pipes[0]: parent writes, child reads (child's stdin)
				// pipes[1]: child writes, parent reads (child's stdout)
				int pipes[2][2] = { 0 };

				if (::pipe(pipes[0]) < 0)
					throw bad_alloc();
				if (::pipe(pipes[1]) < 0)
				{
					::close(pipes[0][0]), ::close(pipes[0][1]);
					throw bad_alloc();
				}

				switch(::fork())
				{
				default:
					// parent
					::close(pipes[0][0]), ::close(pipes[1][1]);
					return make_pair(from_fd(pipes[0][1], "w"), from_fd(pipes[1][0], "r"));

				case -1:
					// Forking error...
					throw runtime_error("forking the process failed");

				case 0:
					// child
					::close(pipes[0][1]), ::close(pipes[1][0]);
					::dup2(pipes[0][0], STDIN_FILENO), ::dup2(pipes[1][1], STDOUT_FILENO);

					auto spawned_path_ = spawned_path;
					auto arguments_ = arguments;
					vector<char *> argv;

					argv.push_back(&spawned_path_[0]);
					for (auto &i : arguments_)
						argv.push_back(&i[0]);
					argv.push_back(nullptr);
					if (::execv(spawned_path.c_str(), argv.data()) < 0)
						exit(-1);
					throw 0;
				}
			}
		}
	}
}
