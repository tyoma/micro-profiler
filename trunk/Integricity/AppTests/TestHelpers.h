#pragma once

#include <string>

namespace ut
{
	std::wstring make_native(System::String ^managed_string);
	System::String ^make_managed(const std::wstring &native_string);
	unsigned long long make_filetime(System::DateTime datetime);

	struct temp_directory
	{
		temp_directory(const std::wstring &path);
		~temp_directory();

		const std::wstring path;
	};

	ref class entries_file
	{
		System::IO::FileStream ^_file;
		System::IO::TextWriter ^_writer;

	public:
		entries_file(const std::wstring &path);
		~entries_file();

		void append(const std::wstring &filename, const std::wstring &revision, System::DateTime modstamp);
		void append_new(const std::wstring &filename);
	};
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
