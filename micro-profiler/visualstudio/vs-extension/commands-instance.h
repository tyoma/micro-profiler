#pragma once

#include <visualstudio/command.h>

#include <atlbase.h>
#include <vsshell.h>

namespace micro_profiler
{
	class functions_list;

	namespace integration
	{
		struct instance_context
		{
			std::shared_ptr<functions_list> model;
			std::wstring executable;
			CComPtr<IVsUIShell> shell;
		};

		typedef command<instance_context> instance_command;

		struct save : instance_command
		{
			save();

			virtual bool query_state(const context_type &ctx, unsigned /*item*/, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned /*item*/);
		};

		struct clear : instance_command
		{
			clear();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct copy : instance_command
		{
			copy();

			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};
	}
}
