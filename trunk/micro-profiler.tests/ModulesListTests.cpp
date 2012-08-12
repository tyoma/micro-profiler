#include <collector/modules_list.h>

#include "Helpers.h"

#include <windows.h>

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace micro_profiler
{
	bool operator ==(const loaded_module &/*lhs*/, const loaded_module &/*rhs*/)
	{
		return false;
	}

	namespace tests
	{
		[TestClass]
		public ref class ModulesListTests
		{
		public: 
			[TestMethod]
			void NonEmptyModulesReturned()
			{
				// INIT
				vector<loaded_module> list;
				
				// ACT
				enumerate_modules(::GetCurrentProcess(), list);
				
				// ASSERT
				Assert::IsFalse(list.empty());
			}


			[TestMethod]
			void ModulesListIsStable()
			{
				// INIT
				vector<loaded_module> list1, list2;
				
				// ACT
				enumerate_modules(::GetCurrentProcess(), list1);
				enumerate_modules(::GetCurrentProcess(), list2);
				
				// ASSERT
				Assert::IsTrue(list2 == list1);
			}


			[TestMethod]
			void LoadedModulesAreListed()
			{
				// INIT
				vector<loaded_module> before, after;

				// ACT
				enumerate_modules(::GetCurrentProcess(), before);

				// ASSERT
			}

		};
	}
}
