#include "fs.h"

#include <windows.h>

using namespace std;

namespace
{
}

namespace fs
{
	entry_type get_entry_type(const wstring &path)
	{
		DWORD attributes = ::GetFileAttributesW(path.c_str());

		return attributes == INVALID_FILE_ATTRIBUTES ? entry_none : 0 != (attributes & FILE_ATTRIBUTE_DIRECTORY) ? entry_directory : entry_file;
	}
}
