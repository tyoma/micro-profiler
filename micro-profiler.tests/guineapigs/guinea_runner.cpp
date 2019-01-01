#include "guinea_runner.h"

#include <common/memory.h>
#include <common/noncopyable.h>
#include <ipc/endpoint.h>
#include <map>
#include <mt/event.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		class command_handler : public ipc::channel, noncopyable
		{
		public:
			command_handler(mt::event &exit, const char *destination_endpoint)
				: _exit(exit)
			{	_commander_channel = ipc::connect_client(destination_endpoint, *this);	}

		private:
			virtual void disconnect() throw()
			{	_exit.set();	}

			virtual void message(const_byte_range payload)
			{
				vector_adapter reader(payload);
				strmd::deserializer<vector_adapter> archive(reader);
				runner_commands c;
				string data1, data2;

				switch (archive(c), c)
				{
				case load_module:
					archive(data1);
					images.insert(make_pair(data1, image(data1.c_str())));
					break;

				case unload_module:
					archive(data1);
					images.erase(data1);
					break;

				case execute_function:
					break;
				}
			}

		private:
			mt::event &_exit;
			map<string, image> images;
			shared_ptr<ipc::channel> _commander_channel;
		};
	}
}

int main(int argc, const char *argv[])
{
	if (argc < 2)
		return -1;

	mt::event exit;
	micro_profiler::tests::command_handler h(exit, argv[1]);

	exit.wait();
	return 0;
}
