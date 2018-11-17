#include <collector/image_patch.h>

using namespace std;

namespace micro_profiler
{
	image_patch::image_patch(const shared_ptr<binary_image> &image, void *instance,
			enter_hook_t *on_enter, exit_hook_t *on_exit)
		: _image(image), _instance(instance), _on_enter(on_enter), _on_exit(on_exit)
	{	}

	void image_patch::apply_for(const filter_t &function_filter)
	{
		_image->enumerate_functions([&] (const function_body &body) {
			if (body.body().length() < 5 || !function_filter(body))
				return;

			shared_ptr<function_patch> p(new function_patch(_allocator, body.effective_address(), body.body(),
				_instance, _on_enter, _on_exit));

			_patches.push_back(p);
		});
	}
}
