#include <test-helpers/constants.h>

#include <common/module.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int g_dummy;
		}

		lazy_load_path::lazy_load_path(const string &name)
			: _name(name)
		{	}

		lazy_load_path::operator string() const
		{	return image(_name).absolute_path();	}

		const string c_this_module = module::locate(&g_dummy).path;
		const lazy_load_path c_symbol_container_1("symbol_container_1");
		const lazy_load_path c_symbol_container_2("symbol_container_2");
		const lazy_load_path c_symbol_container_2_instrumented("symbol_container_2_instrumented");
		const lazy_load_path c_symbol_container_3_nosymbols("symbol_container_3_nosymbols");
	}
}
