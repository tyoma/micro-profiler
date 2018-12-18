#pragma once

#include <test-helpers/helpers.h>

#include <common/symbol_resolver.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class image_info : public micro_profiler::image_info
			{
			public:
				template <typename F>
				void add_function(F *f);

			private:
				virtual void enumerate_functions(const symbol_callback_t &callback) const;

				void add_function(void *f);

			private:
				std::vector<symbol_info> _symbols;
			};



			template <typename F>
			inline void image_info::add_function(F *f)
			{	add_function(address_cast_hack<void *>(f));	}
		}
	}
}
