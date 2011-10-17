#include "Addin.h"
#include "resource.h"

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	//The following #import imports DTE
	#import <dte80a.olb> named_guids

	//The following #import imports DTE80
	#import <dte80.olb> named_guids
#pragma warning(default: 4146)
#pragma warning(default: 4278)

class __declspec(uuid("B36A1712-EF9F-4960-9B33-838BFCC70683")) ProfilerAddin
{
	EnvDTE::_DTEPtr _dte;

public:
	ProfilerAddin(EnvDTE::_DTEPtr dte);
	virtual ~ProfilerAddin();
};

typedef AddinImpl<ProfilerAddin, &__uuidof(ProfilerAddin), IDR_PROFILERADDIN> ProfilerAddinImpl;

OBJECT_ENTRY_AUTO(__uuidof(ProfilerAddin), ProfilerAddinImpl)

ProfilerAddin::ProfilerAddin(EnvDTE::_DTEPtr dte)
{
}

ProfilerAddin::~ProfilerAddin()
{
}
