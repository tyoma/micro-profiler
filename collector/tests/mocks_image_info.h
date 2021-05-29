#pragma once

#include <common/image_info.h>

#include <test-helpers/helpers.h>

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			class image_info : public micro_profiler::image_info<symbol_info_mapped>
			{
			public:
				template <typename F>
				void add_function(F *f);

			private:
				virtual void enumerate_functions(const symbol_callback_t &callback) const override;

				void add_function(void *f);

			private:
				std::vector<symbol_info_mapped> _symbols;
			};



			template <typename F>
			inline void image_info::add_function(F *f)
			{	add_function(address_cast_hack<void *>(f));	}
		}
	}
}
