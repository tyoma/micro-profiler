#include <test-helpers/file_helpers.h>

#include <common/formatting.h>
#include <common/module.h>
#include <common/path.h>
#include <ut/assert.h>

#ifdef _WIN32
	#include <process.h>
	
	#define mkdir _mkdir
	#define rmdir _rmdir
	#define unlink _unlink

	extern "C" int _mkdir(const char *path, int mode);
	extern "C" int _rmdir(const char *path);
	extern "C" int _unlink(const char *path);

#else
	#include <sys/stat.h>
	#include <unistd.h>

#endif

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int dummy;
		}

		temporary_directory::temporary_directory()
		{
			const auto base_path = ~get_module_info(&dummy).path;

			for (unsigned index = 1; index < 1000; ++index)
			{
				string temp = "test_directory.";

				itoa<10>(temp, index, 3);
				temp = base_path & temp;
				if (!::mkdir(temp.c_str(), 777))
				{
					_temp_path = temp;
					return;
				}
				if (EEXIST == errno)
					continue;
				break;
			}
			throw runtime_error("cannot create temporary test directory");
		}

		temporary_directory::~temporary_directory()
		{
			for (auto i = _tracked.begin(); i != _tracked.end(); ++i)
				::unlink((_temp_path & *i).c_str());
			::rmdir(_temp_path.c_str());
		}

		string temporary_directory::path() const
		{	return _temp_path;	}

		string temporary_directory::track_file(const string &filename)
		{
			_tracked.push_back(filename);
			return _temp_path & filename;
		}

		string temporary_directory::copy_file(const string &source)
		{
			const auto file_open = [] (const char *path, const char *mode) -> shared_ptr<FILE> {
				const auto f = fopen(path, mode);

				assert_not_null(f);
				return shared_ptr<FILE>(f, &fclose);
			};
			const auto fsource = file_open(source.c_str(), "rb");
			const auto destination = track_file(*source);
			const auto fdestination = file_open(destination.c_str(), "wb");
			char buffer[1000];

			for (auto bytes = sizeof buffer; bytes == sizeof buffer; )
			{
				bytes = fread(buffer, 1, bytes, fsource.get());
				fwrite(buffer, 1, bytes, fdestination.get());
			}
			return destination;
		}
	}
}
