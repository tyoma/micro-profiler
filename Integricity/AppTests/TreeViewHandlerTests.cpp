#include <treeview_handler.h>

#include "TestHelpers.h"
#include <functional>

using namespace std;
using namespace placeholders;
using namespace System;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace AppTests
{
	[TestClass]
	public ref class TreeViewHandlerTests : ut::WindowTestsBase
	{
	public:
		[TestMethod]
		void FailToWrapNonWindow()
		{
		}
	};
}
