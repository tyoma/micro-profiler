//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include "binary_image.h"

#include <patcher/function_patch.h>

namespace micro_profiler
{
	class image_patch : noncopyable
	{
	public:
		typedef std::function<bool (const function_body &body)> filter_t;

	public:
		template <typename InterceptorT>
		image_patch(const std::shared_ptr<binary_image> &image, InterceptorT *interceptor);

		void apply_for(const filter_t &function_filter);

	private:
		const std::shared_ptr<binary_image> _image;
		void * const _interceptor;
		hooks<void>::on_enter_t * const _on_enter;
		hooks<void>::on_exit_t * const _on_exit;
		std::vector< std::shared_ptr<function_patch> > _patches;
		executable_memory_allocator _allocator;
	};

	
	
	template <typename InterceptorT>
	inline image_patch::image_patch(const std::shared_ptr<binary_image> &image, InterceptorT *interceptor)
		: _image(image), _interceptor(interceptor),
			_on_enter(hooks<InterceptorT>::on_enter()), _on_exit(hooks<InterceptorT>::on_exit())
	{	}
}
