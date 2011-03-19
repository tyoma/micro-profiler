#pragma once

#include "basics.h"
#include "windowing.h"

#include <memory>

#pragma warning(disable: 4278)
#pragma warning(disable: 4146)
	//The following #import imports DTE
	#import <dte80a.olb> named_guids

	//The following #import imports DTE80
	#import <dte80.olb> named_guids
#pragma warning(default: 4146)
#pragma warning(default: 4278)

class application : public destructible
{
	EnvDTE::_DTEPtr _dte;

	std::shared_ptr<destructible> _openedConnection, _closedConnection;
	std::shared_ptr<destructible> _interception;

public:
	application(EnvDTE::_DTEPtr dte);
	~application();
};
