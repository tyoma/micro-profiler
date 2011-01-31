#pragma once

#include <string>

namespace ut
{
	std::wstring make_native(System::String ^managed_string);
	System::String ^make_managed(const std::wstring &native_string);

	struct temp_directory
	{
		temp_directory(const std::wstring &path);
		~temp_directory();

		const std::wstring path;
	};
}

#define ASSERT_THROWS(fragment, expected_exception) try { fragment; Assert::Fail("Expected exception was not thrown!"); } catch (const expected_exception &) { } catch (...) { Assert::Fail("Exception of unexpected type was thrown!"); }
