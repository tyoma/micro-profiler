#include "filemapping.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

namespace symreader
{
	namespace
	{
		struct mapping : mapped_region
		{
			mapping(const char *path)
				: mapped_region(0, 0)
			{
				if ((file = ::open(path, O_RDONLY)) >= 0)
				{
					struct stat st;

					if (::fstat(file, &st) >= 0)
					{
						second = static_cast<size_t>(st.st_size);
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
