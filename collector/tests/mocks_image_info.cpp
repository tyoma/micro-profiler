#include "mocks_image_info.h"

#include <common/symbol_resolver.h>
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
				for (vector<symbol_info>::const_iterator i = _symbols.begin(); i != _symbols.end(); ++i)
					callback(*i);
			}

			void image_info::add_function(void *f)
			{
				bool done = false;
				module_info mi = get_module_info(f);
				shared_ptr<micro_profiler::image_info> ii(new offset_image_info(micro_profiler::image_info::load(
					mi.path.c_str()), static_cast<size_t>(mi.load_address)));

				ii->enumerate_functions([&] (const symbol_info &symbol) {
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
