#pragma once

#include <functional>

namespace testing
{
	public ref class AssertEx
	{
	public:
		template <typename ExpectedException>
		static ExpectedException Throws(const std::function<void()> &fragment)
		{
			try
			{
				fragment();
				Microsoft::VisualStudio::TestTools::UnitTesting::Assert::Fail("No exception was thrown!");
			}
			catch (const ExpectedException &)
			{
				return e;
			}
			catch (...)
			{
				Microsoft::VisualStudio::TestTools::UnitTesting::Assert::Fail("Exception of unexepected type was thrown!");
			}
		}
	};
}
