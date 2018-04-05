#include "stream.h"

using namespace std;

namespace micro_profiler
{
	write_stream::write_stream(const wstring &path)
		: _file(_wfopen(path.c_str(), L"wb+"), &::fclose)
	{	}

	void write_stream::write(const byte *buffer, size_t size)
	{	fwrite(buffer, 1, size, static_cast<FILE *>(_file.get()));	}

	read_stream::read_stream(const wstring &path)
		: _file(_wfopen(path.c_str(), L"rb"), &::fclose)
	{	}

	void read_stream::read(byte *buffer, size_t size)
	{
		if (size != fread(buffer, 1, size, static_cast<FILE *>(_file.get())))
			throw runtime_error("Reading past end the file!");
	}
}
