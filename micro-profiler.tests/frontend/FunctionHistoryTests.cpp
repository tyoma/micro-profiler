#include <frontend/function_history.h>
#include <frontend/function_list.h>

#include "Mockups.h"

#include <common/com_helpers.h>

namespace std
{
	using namespace tr1;
}

using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			__int64 test_ticks_resolution = 1;
		}

		[TestClass]
		public ref class FunctionHistoryTests
		{
			shared_ptr<functions_list>* functions_ptr;
			functions_list* functions;

		public:
			[TestInitialize]
			void CreateFunctionsList()
			{
				shared_ptr<symbol_resolver> r(new mocks::sri);
				functions_ptr = new shared_ptr<functions_list>(functions_list::create(test_ticks_resolution, r));
				functions = functions_ptr->get();
			}

			[TestCleanup]
			void DestroyFunctionsList()
			{
				delete functions_ptr;
				functions = 0;
			}


			[TestMethod]
			void FunctionHistoryRangeIsSingleTimePointForInitiallyUpdatedFunctions()
			{
				// INIT
				FunctionStatisticsDetailed s[2] = { 0 };

				copy(make_pair((void *)1123, function_statistics(19, 0, 31, 29)), s[0].Statistics);
				copy(make_pair((void *)11231, function_statistics(20, 0, 317, 11129)), s[1].Statistics);

				functions->set_order(1 /*# calls*/, true);

				// ACT
				functions->update(&s[0], 1);
				shared_ptr<function_history> h1 = functions->get_history(0);

				// ASSERT
				Assert::IsTrue(!!h1);

				// ACT
				function_history::time_range r = h1->get_range();

				Assert::IsTrue(712311231 == r.first);
				Assert::IsTrue(712311231 == r.second);
			}
		};
	}
}
