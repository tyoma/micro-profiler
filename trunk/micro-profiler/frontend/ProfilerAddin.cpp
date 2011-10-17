#include "Addin.h"
#include "resource.h"

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52"	// mso.dll
	#import <dte80a.olb>
	#import <dte80.olb>
#pragma warning(default: 4146)
#pragma warning(default: 4278)

class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin
{
	EnvDTE::_DTEPtr _dte;
	Office::_CommandBarsPtr _projectContextMenus;

public:
	ProfilerAddin(EnvDTE::_DTEPtr dte);
	virtual ~ProfilerAddin();
};

typedef AddinImpl<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;

OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl)

ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr dte)
	: _dte(dte)
{
//	Office::CommandBarPtr contextMenus = Office::_CommandBarsPtr(_dte->CommandBars)->Item[L"Context Menus"];

//	_projectContextMenus = contextMenus->Controls->Item[L"Project and Solution Context Menus"];
}

ProfilerAddin::~ProfilerAddin()
{
}
