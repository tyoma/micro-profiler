#pragma once

#include "../display.h"

typedef struct HWND__ *HWND;

namespace charting
{
	std::shared_ptr<display> create_display(HWND hwnd);
}
