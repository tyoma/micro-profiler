#include "file.h"

#include <common/string.h>
#include <Shlwapi.h>

using namespace std;

namespace micro_profiler
{
	const wchar_t c_microProfilerFileExtension[] = L"mpstats";
	const wchar_t c_microProfilerFiles[] = L"MicroProfiler Statistics\0*.mpstats\0\0";

	auto_ptr<write_stream> create_file(HWND hparent, const string &default_name)
	{
		auto_ptr<write_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };
		wstring default_name2 = unicode(default_name);
		
		default_name2.append(L".").append(c_microProfilerFileExtension);

		copy(default_name2.begin(), default_name2.end(), buffer);
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hparent;
		ofn.lpstrFilter = c_microProfilerFiles;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerFileExtension;
		if (::GetSaveFileNameW(&ofn))
			r.reset(new write_stream(ofn.lpstrFile));
		return r;
	}

	auto_ptr<read_stream> open_file(HWND hparent, string& path)
	{
		auto_ptr<read_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hparent;
		ofn.lpstrFilter = c_microProfilerFiles;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerFileExtension;
		if (::GetOpenFileNameW(&ofn))
			r.reset(new read_stream(ofn.lpstrFile)), path = unicode(ofn.lpstrFile);
		return r;
	}
}
