#pragma once

#include "basics.h"

namespace mt
{
	struct waitable : destructible
	{
		static const unsigned int infinite = -1;

		virtual void wait(unsigned int timeout) = 0;
	};
}
