#include "TestHelpers.h"

#include <fs.h>
#include <vcclr.h>

using namespace std;
using namespace fs;
using namespace System;
using namespace System::IO;
using namespace System::Globalization;

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
	{
		datetime.AddYears(1600);
		return datetime.Ticks;
	}

	temp_directory::temp_directory(const wstring &path_)
		: path(path_)
	{
		Directory::CreateDirectory(make_managed(path));
	}

	temp_directory::~temp_directory()
	{
		Directory::Delete(make_managed(path), true);
	}


	entries_file::entries_file(const wstring &repository_directory)
	{
		Directory::CreateDirectory(make_managed(repository_directory / L"cvs"));
		_file = File::Create(make_managed(repository_directory / L"cvs/entries"));
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
}
