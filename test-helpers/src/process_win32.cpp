#include <test-helpers/process.h>

#include <stdexcept>
#include <vector>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class running_process_impl : public running_process
			{
			public:
				running_process_impl(const string &executable_path, const string &command_line)
				{
					STARTUPINFOA si = { };
					string path = executable_path;
					vector<char> cl2(command_line.begin(), command_line.end());

					if (path.find(".exe") == string::npos)
						path += ".exe";
					cl2.push_back(char());

					si.cb = sizeof(si);
					if (!::CreateProcessA(path.c_str(), &cl2[0], NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS , NULL,
						NULL, &si, &_process))
					{
						DWORD err = ::GetLastError();
						err;
						throw runtime_error("Cannot run child process!");
					}
				}

				~running_process_impl()
				{
					::CloseHandle(_process.hThread);
					::CloseHandle(_process.hProcess);
				}

				virtual unsigned int get_pid() const
				{	return _process.dwProcessId;	}

				virtual void wait() const
				{	::WaitForSingleObject(_process.hProcess, INFINITE);	}

			private:
				PROCESS_INFORMATION _process;
			};
		}

		shared_ptr<running_process> create_process(const string &executable_path, const string &command_line)
		{	return shared_ptr<running_process>(new running_process_impl(executable_path, command_line));	}
	}
}
