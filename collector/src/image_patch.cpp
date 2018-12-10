#include <collector/image_patch.h>

#include <set>

using namespace std;

namespace micro_profiler
{
	void image_patch::apply_for(const filter_t &function_filter)
	{
		set<const void *> patched;

		_image->enumerate_functions([&] (const symbol_info &symbol) {
			try
			{
				if (!patched.insert(symbol.body.begin()).second)
					return;
				if (symbol.body.length() < 5 || !function_filter(symbol))
					return;

				shared_ptr<function_patch> p(new function_patch(_allocator, symbol.body,
					_interceptor, _on_enter, _on_exit));

				_patches.push_back(p);
			}
			catch (exception &/*e*/)
			{
			}
		});
	}
}
