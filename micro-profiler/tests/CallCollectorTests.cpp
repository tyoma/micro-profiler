#include <calls_collector.h>

#include "Helpers.h"

using namespace System;
using namespace	Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		void sleep_20();

		[TestClass]
		public ref class CallCollectorTests
		{
		public:
			[TestMethod]
			void CollectEntryExitOnSimpleExternalFunction()
			{
				// ACT
				sleep_20();

				// ASSERT
//				calls_collector::instance()->read_collected();
			}
		};
	}
}
