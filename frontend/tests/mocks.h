#pragma once

#include <common/protocol.h>
#include <frontend/symbol_resolver.h>
#include <frontend/tables.h>
#include <frontend/threads_model.h>

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

			class threads_model : public micro_profiler::threads_model
			{
			public:
				threads_model();

				void add(unsigned int thread_id, unsigned int native_id, const std::string &description);
			};



			template <typename T, size_t n>
			inline std::shared_ptr<symbol_resolver> symbol_resolver::create(T (&symbols)[n])
			{
				auto modules = std::make_shared<tables::modules>();
				auto &m1 = (*modules)[static_cast<unsigned>(-1)];
				auto mappings = std::make_shared<tables::module_mappings>();

				for (size_t i = 0; i != n; ++i)
				{
					symbol_info symbol = { symbols[i].second, static_cast<unsigned>(symbols[i].first), 1, };

					m1.symbols.push_back(symbol);
				}

				(*mappings)[0u].instance_id = 0;
				(*mappings)[0u].persistent_id = static_cast<unsigned>(-1);
				(*mappings)[0u].base = 0;
				return std::make_shared<mocks::symbol_resolver>(modules, mappings);
			}
		}
	}
}
