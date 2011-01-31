#include "TestHelpers.h"

#include <vcclr.h>

using namespace std;
using namespace System;
using namespace System::IO;

namespace ut
{
	wstring make_native(String ^managed_string)
	{
		pin_ptr<const wchar_t> buffer(PtrToStringChars(managed_string));

		return wstring(buffer);
	}

	String ^make_managed(const wstring &native_string)
	{	return gcnew String(native_string.c_str());	}


	temp_directory::temp_directory(const wstring &path_)
		: path(path_)
	{
		Directory::CreateDirectory(make_managed(path));
	}

	temp_directory::~temp_directory()
	{
		Directory::Delete(make_managed(path), true);
	}
}
