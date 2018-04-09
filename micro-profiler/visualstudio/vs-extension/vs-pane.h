#pragma once

#include <memory>
#include <string>
#include <vsshell.h>

namespace micro_profiler
{
	struct frontend_ui;
	class functions_list;

	std::shared_ptr<frontend_ui> create_ui(IVsUIShell &shell, unsigned id, const std::shared_ptr<functions_list> &model,
		const std::wstring &executable);
}
