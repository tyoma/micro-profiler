#pragma once

#include <string>

namespace micro_profiler
{
	namespace tests
	{
		class lazy_load_path
		{
		public:
			explicit lazy_load_path(const std::string &name);

			operator std::string() const;

		private:
			std::string _name;
		};

		extern const std::string c_this_module;
		extern const lazy_load_path c_symbol_container_1;
		extern const lazy_load_path c_symbol_container_2;
		extern const lazy_load_path c_symbol_container_2_instrumented;
		extern const lazy_load_path c_symbol_container_3_nosymbols;
	}
}
