#pragma once

#include "basics.h"

#include <memory>

class treeview_handler : noncopyable
{
	std::shared_ptr<destructible> _interception;

public:
	treeview_handler(void *htree);
};
