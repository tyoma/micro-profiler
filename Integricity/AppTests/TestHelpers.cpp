#include "TestHelpers.h"

#include <windows.h>
#include <fs.h>
#include <vcclr.h>

using namespace std;
using namespace fs;
using namespace System;
using namespace System::IO;
using namespace System::Globalization;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace ut
{
	wstring make_native(String ^managed_string)
	{
		pin_ptr<const wchar_t> buffer(PtrToStringChars(managed_string));

		return wstring(buffer);
	}

	String ^make_managed(const wstring &native_string)
	{	return gcnew String(native_string.c_str());	}

	unsigned long long make_filetime(DateTime datetime)
	{	return datetime.AddYears(-1600).Ticks;	}

	temp_directory::temp_directory(const wstring &name)
		: _path(Path::IsPathRooted(make_managed(name)) ? make_managed(name) : make_managed(make_native(_temp_root) / name))
	{
		Directory::CreateDirectory(_path);
	}

	temp_directory::~temp_directory()
	{
		Directory::Delete(_path, true);
	}

	wstring temp_directory::path()
	{	return make_native(_path);	}


	entries_file::entries_file(const wstring &repository_directory)
	{
		Directory::CreateDirectory(make_managed(repository_directory / L"CVS"));
		_file = File::Create(make_managed(repository_directory / L"CVS/entries"));
		_writer = gcnew StreamWriter(_file);
	}

	entries_file::~entries_file()
	{
		_writer->WriteLine("D");
		delete _writer;
		delete _file;
	}

	void entries_file::append(const wstring &filename, const wstring &revision, DateTime modstamp)
	{
		_writer->WriteLine(String::Format(CultureInfo::InvariantCulture, "{0}/{1}/{2}/{3:ddd MMM dd HH:mm:ss yyyy}/{4}/{5}",
			"",
			make_managed(filename),
			make_managed(revision),
			modstamp,
			"",
			""));
	}

	void entries_file::append_new(const wstring &filename)
	{
		_writer->WriteLine(String::Format(CultureInfo::InvariantCulture, "{0}/{1}/{2}/{3:}/{4}/{5}",
			"",
			make_managed(filename),
			"0",
			"dummy timestamp",
			"",
			""));
	}


	WindowTestsBase::WindowTestsBase()
		: _windows(new vector<void *>())
	{	}

	WindowTestsBase::~WindowTestsBase()
	{	delete _windows;	}

	void *WindowTestsBase::create_window()
	{	return create_window(_T("static"));	}

	void *WindowTestsBase::create_window(const TCHAR *class_name)
	{
		HWND hwnd = ::CreateWindow(class_name, NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, NULL, NULL);

		_windows->push_back(hwnd);
		return hwnd;
	}

	void *WindowTestsBase::create_tree()
	{
		HWND hparent = ::CreateWindow(_T("static"), NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
		HWND htree = ::CreateWindow(_T("systreeview32"), NULL, WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hparent, NULL, NULL, NULL);

		_windows->push_back(htree);
		_windows->push_back(hparent);
		return htree;
	}

	void WindowTestsBase::cleanup()
	{
		for (vector<void *>::const_iterator i = _windows->begin(); i != _windows->end(); ++i)
			if (::IsWindow(reinterpret_cast<HWND>(*i)))
				::DestroyWindow(reinterpret_cast<HWND>(*i));
	}
}
