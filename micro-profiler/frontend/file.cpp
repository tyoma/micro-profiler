#include "file.h"

#include <Shlwapi.h>

using namespace std;

namespace micro_profiler
{
	const wchar_t c_microProfilerFileExtension[] = L"mpstats";
	const wchar_t c_microProfilerFiles[] = L"MicroProfiler Statistics\0*.mpstats\0\0";

	write_stream::write_stream(const wstring &path)
		: _file(_wfopen(path.c_str(), L"wb+"), &::fclose)
	{	}

	void write_stream::write(const byte *buffer, size_t size)
	{	fwrite(buffer, 1, size, _file.get());	}

	read_stream::read_stream(const wstring &path)
		: _file(_wfopen(path.c_str(), L"rb"), &::fclose)
	{	}

	void read_stream::read(byte *buffer, size_t size)
	{
		if (size != fread(buffer, 1, size, _file.get()))
			throw runtime_error("Reading past end the file!");
	}

	auto_ptr<write_stream> create_file(const wstring &default_name)
	{
		auto_ptr<write_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };
		wstring default_name2 = default_name;
		
		default_name2.append(L".").append(c_microProfilerFileExtension);

		copy(default_name2.begin(), default_name2.end(), buffer);
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = c_microProfilerFiles;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerFileExtension;
		if (::GetSaveFileNameW(&ofn))
			r.reset(new write_stream(ofn.lpstrFile));
		return r;
	}

	auto_ptr<read_stream> open_file(wstring& path)
	{
		auto_ptr<read_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };

		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = c_microProfilerFiles;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerFileExtension;
		if (::GetOpenFileNameW(&ofn))
			r.reset(new read_stream(ofn.lpstrFile)), path = ofn.lpstrFile;
		return r;
	}
}
