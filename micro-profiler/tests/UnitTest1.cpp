#include "Helpers.h"

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace	Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class TF : public thread_function
			{
				virtual void operator ()()
				{
				}
			};
		}

		[TestClass]
		public ref class UnitTest1
		{
		public:
			[TestMethod]
			void TestMethod1()
			{
				TF f;
				thread t(f);
			};
		};
	}
}