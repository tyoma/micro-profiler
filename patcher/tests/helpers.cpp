#include "helpers.h"

#include <common/module.h>
#include <common/image_info.h>
#include <ut/assert.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		byte_range get_function_body(void *f)
		{
			mapped_module mi = get_module_info(f);
			shared_ptr< image_info<symbol_info_mapped> > img(new offset_image_info(load_image_info(mi.path.c_str()),
				reinterpret_cast<size_t>(mi.base)));
			byte_range body(0, 0);

			img->enumerate_functions([&] (const symbol_info_mapped &symbol) {
				if (symbol.body.begin() == f)
					body = symbol.body;
			});
			assert_is_true(body.length() > 0);
			return body;
		}
	}
}
