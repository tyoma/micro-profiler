#pragma once

#include "command-ids.h"

#include <wpl/vs/command.h>

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
			std::string executable;
			CComPtr<IVsUIShell> shell;
		};


		struct pause_updates : command_defined<instance_context, cmdidPauseUpdates>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct resume_updates : command_defined<instance_context, cmdidResumeUpdates>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct save : command_defined<instance_context, cmdidSaveStatistics>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct clear : command_defined<instance_context, cmdidClearStatistics>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};

		struct copy : command_defined<instance_context, cmdidCopyStatistics>
		{
			virtual bool query_state(const context_type &ctx, unsigned item, unsigned &state) const;
			virtual void exec(context_type &ctx, unsigned item);
		};
	}
}
