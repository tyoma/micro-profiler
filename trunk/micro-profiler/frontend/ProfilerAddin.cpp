#include "Addin.h"
#include "resource.h"

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import <vsmso.olb>
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" no_implementation // mso.dll
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" implementation_only
	#import <dte80a.olb> no_implementation
	#import <dte80a.olb> implementation_only
#pragma warning(default: 4146)
#pragma warning(default: 4278)

using namespace EnvDTE;

class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin
{
	_DTEPtr _dte;

public:
	ProfilerAddin(_DTEPtr dte);
	virtual ~ProfilerAddin();

	static void initialize(_DTEPtr dte, AddInPtr addin);
};

typedef AddinImpl<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;

OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl)

ProfilerAddin::ProfilerAddin(_DTEPtr dte)
	: _dte(dte)
{
}

ProfilerAddin::~ProfilerAddin()
{
}

template <typename CommandBarPtrT, typename CommandBarButtonPtrT, typename CommandBarsPtrT, int commands_count>
void setup_menu(CommandBarsPtrT commandbars, CommandPtr (&new_commands)[commands_count])
{
	CommandBarPtrT targetCommandBar(commandbars->Item[L"Project"]);
	long position = targetCommandBar->Controls->Count + 1;

	for (int i = 0; i < commands_count; ++i, ++position)
	{
		CommandBarButtonPtrT item(new_commands[i]->AddControl(targetCommandBar, position));

		if (0 == i)
			item->BeginGroup = true;
	}
}

void ProfilerAddin::initialize(_DTEPtr dte, AddInPtr addin)
{
	CommandsPtr commands(dte->Commands);
	CommandPtr new_commands[] = {
		commands->AddNamedCommand(addin, L"AddInstrumentation", L"Add Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
		commands->AddNamedCommand(addin, L"RemoveInstrumentation", L"Remove Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
		commands->AddNamedCommand(addin, L"ResetInstrumentation", L"Reset Instrumentation", L"", VARIANT_TRUE, 0, NULL, 16),
		commands->AddNamedCommand(addin, L"RemoveProfilingSupport", L"Remove Profiling Support", L"", VARIANT_TRUE, 0, NULL, 16),
	};

	if (Microsoft_VisualStudio_CommandBars::_CommandBarsPtr cb = dte->CommandBars)
		setup_menu<Microsoft_VisualStudio_CommandBars::CommandBarPtr, Microsoft_VisualStudio_CommandBars::_CommandBarButtonPtr>(cb, new_commands);
	else if (Office::_CommandBarsPtr cb = dte->CommandBars)
		setup_menu<Office::CommandBarPtr, Office::_CommandBarButtonPtr>(cb, new_commands);
}
