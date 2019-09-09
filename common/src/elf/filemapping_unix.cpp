#include "filemapping.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

using namespace std;

namespace symreader
{
	namespace
	{
		size_t get_file_size(const char *path)
		{
			if (FILE *f = fopen(path, "rb"))
			{
				fseek(f, 0, SEEK_END);
				size_t s = ftell(f);
				fseek(f, 0, SEEK_SET);
				fclose(f);
				return s;
			}
			return 0;
		}

		struct mapping : mapped_region
		{
			mapping(const char *path)
				: mapped_region(0, 0)
			{
				if ((file = ::open(path, O_RDONLY)) >= 0)
				{
					if (second = get_file_size(path), second)
					{
						first = ::mmap(0, second, PROT_READ, MAP_PRIVATE, file, 0);
						if (first != MAP_FAILED)
							return;
						first = 0, second = 0;
					}
					close(file);
					file = -1;
				}
			}

			~mapping()
			{
				if (first)
					::munmap((void *)first, second);
				if (-1 != file)
					::close(file);
			}

			int file;
		};
	}

	shared_ptr<const mapped_region> map_file(const char *path)
	{	return shared_ptr<mapping>(new mapping(path));	}
}
