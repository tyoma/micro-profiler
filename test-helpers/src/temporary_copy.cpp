#include <test-helpers/temporary_copy.h>

#include <common/path.h>
#include <fcntl.h>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <ut/assert.h>

#if WIN32
	#include <io.h>
	#include <sys/stat.h>
	#define O_EX _S_IREAD|_S_IWRITE
#else
	#include <unistd.h>
	#define O_EX 0
	#define O_BINARY 0
#endif

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		temporary_copy::temporary_copy(const string &source)
		{
			auto source_file = shared_ptr<FILE>(fopen(source.c_str(), "rb"), &fclose);
			auto generate_name = [source] (long long u) {
				return ~source & (to_string(u) + "_" + *source);
			};

			for (auto i = 0; i < 10000; ++i)
			{
				_path = generate_name(i);

				auto handle = open(_path.c_str(), O_BINARY | O_WRONLY | O_CREAT | O_EXCL, O_EX);

				if (handle > 0)
				{
					char buffer[8192];
					size_t bytes;

					do
					{
						bytes = fread(buffer, 1, sizeof buffer, source_file.get());

						assert_equal(bytes, static_cast<size_t>(write(handle, buffer, static_cast<unsigned int>(bytes))));
					} while (bytes == sizeof buffer);
					close(handle);
					return;
				}
			}
			throw runtime_error("Cannot initialize new file!");
		}

		temporary_copy::~temporary_copy()
		{	unlink(_path.c_str());	}
	}
}
