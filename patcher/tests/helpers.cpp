#include "helpers.h"

#include <common/module.h>
#include <common/symbol_resolver.h>
#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		byte_range get_function_body(void *f)
		{
			shared_ptr<symbol_resolver> r = symbol_resolver::create();
			module_info mi = get_module_info(f);
			size_t body_size = 0;

			r->add_image(mi.path.c_str(), mi.load_address);
			r->enumerate_symbols((size_t)mi.load_address, [&] (const symbol_info &symbol) {
				if (symbol.location == f)
					body_size = symbol.size;
			});
			assert_is_true(body_size > 0);
			return byte_range((byte *)f, body_size);
		}
	}
}
