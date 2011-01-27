#include "application.h"

#include "repository.h"
#include "dispatch.h"

using namespace std;

namespace
{
	void f()
	{
	}
}

application::application(EnvDTE::_DTEPtr dte)
	: _dte(dte)
{
	_openedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(EnvDTE::_dispSolutionEvents), 1, &f);
	_closedConnection = connection::make(_dte->Events->SolutionEvents, __uuidof(EnvDTE::_dispSolutionEvents), 1, &f);
}


application::~application()
{
}
