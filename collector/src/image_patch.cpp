#include <collector/image_patch.h>

using namespace std;

namespace micro_profiler
{
	image_patch::patch_entry::patch_entry(symbol_info_mapped symbol, const shared_ptr<function_patch> &patch)
		: _symbol(symbol), _patch(patch)
	{	}

	const symbol_info_mapped &image_patch::patch_entry::get_symbol() const
	{	return _symbol;	}

	void image_patch::apply_for(const filter_t &filter)
	{
		_image->enumerate_functions([&] (const symbol_info_mapped &symbol) {
			try
			{
				if (_patches.find(symbol.body.begin()) == _patches.end() && symbol.body.length() >= 5 && filter(symbol))
				{
					shared_ptr<function_patch> p(new function_patch(_allocator, symbol.body,
						_interceptor, _on_enter, _on_exit));

					_patches.insert(make_pair(symbol.body.begin(), image_patch::patch_entry(symbol, p)));
				}
			}
			catch (exception &/*e*/)
			{
			}
		});

		for (patches_container_t::iterator i = _patches.begin(), j = i; i != _patches.end() ? j = i++, true : false; )
		{
			if (!filter(j->second.get_symbol()))
				_patches.erase(j);
		}
	}
}
