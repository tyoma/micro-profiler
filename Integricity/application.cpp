#include "application.h"

#include "dispatch.h"

namespace
{
	void f()
	{
	}
}

application::application(EnvDTE::_DTEPtr dte)
	: _dte(dte)
{
	_openedConnection = connection::connect(_dte->Events->SolutionEvents, __uuidof(EnvDTE::_dispSolutionEvents), 1, &f);
	_closedConnection = connection::connect(_dte->Events->SolutionEvents, __uuidof(EnvDTE::_dispSolutionEvents), 1, &f);
}


application::~application()
{
}
