#include "mocks_image_info.h"

#include <common/module.h>
#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			void image_info::enumerate_functions(const symbol_callback_t &callback) const
			{
				for (vector<symbol_info_mapped>::const_iterator i = _symbols.begin(); i != _symbols.end(); ++i)
					callback(*i);
			}

			void image_info::add_function(void *f)
			{
				bool done = false;
				mapped_module mi = get_module_info(f);
				shared_ptr< micro_profiler::image_info<symbol_info_mapped> > ii(new offset_image_info(micro_profiler::load_image_info(
					mi.path.c_str()), (size_t)mi.base));

				ii->enumerate_functions([&] (const symbol_info_mapped &symbol) {
					if (symbol.body.begin() == f && !done)
					{
						_symbols.push_back(symbol);
						done = true;
					}
				});
				assert_is_true(done);
			}
		}
	}
}
