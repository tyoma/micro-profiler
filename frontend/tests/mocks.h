#pragma once

#include <common/protocol.h>
#include <frontend/symbol_resolver.h>
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
				symbol_resolver();

				virtual const std::string &symbol_name_by_va(long_address_t address) const;

				static std::string to_string_address(long_address_t value);

				template <typename T, size_t n>
				static std::shared_ptr<symbol_resolver> create(T (&symbols)[n]);

			private:
				mutable std::unordered_map<long_address_t, std::string> _names;
			};

			class threads_model : public micro_profiler::threads_model
			{
			public:
				void add(unsigned int thread_id, unsigned int native_id, const std::string &description);
			};



			template <typename T, size_t n>
			inline std::shared_ptr<symbol_resolver> symbol_resolver::create(T (&symbols)[n])
			{
				std::shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module_identified basic = { };
				module_info_metadata metadata;

				for (size_t i = 0; i != n; ++i)
				{
					symbol_info symbol = { symbols[i].second, static_cast<unsigned>(symbols[i].first), 1, };

					metadata.symbols.push_back(symbol);
				}
				basic.persistent_id = static_cast<unsigned>(-1);
				r->add_mapping(basic);
				r->add_metadata(basic.persistent_id, metadata);
				return r;
			}
		}
	}
}
