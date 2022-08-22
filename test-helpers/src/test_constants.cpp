#include <test-helpers/constants.h>

#include <common/module.h>
#include <common/path.h>
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

		const string c_this_module = module::locate(&g_dummy).path;
		const string c_symbol_container_1 = ~c_this_module & normalize::lib("symbol_container_1");
		const string c_symbol_container_2 = ~c_this_module & normalize::lib("symbol_container_2");
		const string c_symbol_container_2_instrumented = ~c_this_module & normalize::lib("symbol_container_2_instrumented");
		const string c_symbol_container_3_nosymbols = ~c_this_module & normalize::lib("symbol_container_3_nosymbols");

		const std::string c_guinea_ipc_spawn = ~c_this_module & normalize::exe("guinea_ipc_spawn");
		const std::string c_guinea_ipc_spawn_server = ~c_this_module & normalize::exe("guinea_ipc_spawn_server");
		const std::string c_guinea_runner = ~c_this_module & normalize::exe("guinea_runner");
		const std::string c_guinea_runner2 = ~c_this_module & normalize::exe("guinea_runner2");
		const std::string c_guinea_runner3 = ~c_this_module & normalize::exe("guinea_runner3");

	}
}
