#pragma once

#include "../Helpers.h"

#include <frontend/symbol_resolver.h>

#include <map>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class sri : public symbol_resolver
			{
				mutable std::map<const void *, std::wstring> _names;

			public:
				virtual const std::wstring &symbol_name_by_va(const void *address) const
				{
					return _names[address] = to_string(address);
				}

				virtual void add_image(const wchar_t * /*image*/, const void * /*base*/)
				{
				}
			};
		}
	}
}
