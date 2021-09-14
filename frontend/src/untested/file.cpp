//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#include <frontend/file.h>

#include <common/string.h>
#include <Shlwapi.h>

using namespace std;

namespace micro_profiler
{
	const wchar_t c_microProfilerDefaultExtension[] = L"mpstat";
	const wchar_t c_microProfilerFilesSave[] = L"MicroProfiler Statistics\0*.mpstat\0\0";
	const wchar_t c_microProfilerFilesOpen[] = L"MicroProfiler Statistics\0*.mpstat3;*.mpstat4;*.mpstat\0\0";

	unique_ptr<write_stream> create_file(HWND hparent, const string &default_name)
	{
		unique_ptr<write_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };
		wstring default_name2 = unicode(default_name);
		
		default_name2.append(L".").append(c_microProfilerDefaultExtension);

		copy(default_name2.begin(), default_name2.end(), buffer);
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hparent;
		ofn.lpstrFilter = c_microProfilerFilesSave;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerDefaultExtension;
		if (::GetSaveFileNameW(&ofn))
			r.reset(new write_stream(ofn.lpstrFile));
		return r;
	}

	unique_ptr<read_stream> open_file(HWND hparent, string& path)
	{
		unique_ptr<read_stream> r;
		OPENFILENAMEW ofn = {};
		wchar_t buffer[1000] = { 0 };

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hparent;
		ofn.lpstrFilter = c_microProfilerFilesOpen;
		ofn.lpstrFile = buffer;
		ofn.nMaxFile = _countof(buffer);
		ofn.lpstrDefExt = c_microProfilerDefaultExtension;
		if (::GetOpenFileNameW(&ofn))
			r.reset(new read_stream(ofn.lpstrFile)), path = unicode(ofn.lpstrFile);
		return r;
	}
}
