#include <crtdbg.h>

#include <frontend/frontend_manager_impl.h>

using namespace std;

namespace micro_profiler
{
	shared_ptr<frontend_ui> frontend_manager_impl::default_ui_factory(const shared_ptr<functions_list>&, const wstring&)
	{	throw 0;	}

	namespace
	{
		class Module : public CAtlDllModuleT<Module> { } g_module;

		class Initializer
		{
		public:
			Initializer()
			{
				_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
				::CoInitialize(0);
			}

			~Initializer()
			{	::CoUninitialize();	}

		} g_initializer;
	}
}
