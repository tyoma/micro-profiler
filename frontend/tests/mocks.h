#pragma once

#include <common/protocol.h>
#include <frontend/symbol_resolver.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class symbol_resolver : public micro_profiler::symbol_resolver
			{
			public:
				symbol_resolver(std::shared_ptr<tables::modules> modules, std::shared_ptr<tables::module_mappings> mappings);

				virtual const std::string &symbol_name_by_va(long_address_t address) const;

				static std::string to_string_address(long_address_t value);

				template <typename T, size_t n>
				static std::shared_ptr<symbol_resolver> create(T (&symbols)[n]);

			private:
				mutable containers::unordered_map<long_address_t, std::string> _names;
			};
		}
	}
}
