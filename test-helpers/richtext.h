#pragma once

#include <agge.text/richtext.h>

namespace micro_profiler
{
	namespace tests
	{
		inline agge::richtext_modifier_t T(const std::string& from)
		{
			agge::richtext_modifier_t text(agge::style_modifier::empty);

			text << from.c_str();
			return text;
		}
	}
}
