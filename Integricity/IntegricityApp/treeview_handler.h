#pragma once

#include "basics.h"

#include <functional>
#include <memory>
#include <vector>

typedef std::function<void (
	const std::vector<std::wstring> &/*item_path*/,
	unsigned int &/*foreground_color*/,
	unsigned int &/*background_color*/,
	bool &/*emphasized*/
)> prepaint_handler_t;

std::shared_ptr<destructible> handle_tv_notifications(void *htree, const prepaint_handler_t &prepaint_handler);
