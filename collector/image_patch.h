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

#include <common/symbol_resolver.h>
#include <patcher/function_patch.h>
#include <unordered_map>

namespace micro_profiler
{
	class image_patch : noncopyable
	{
	public:
		typedef std::function<bool (const symbol_info &function)> filter_t;

	public:
		template <typename InterceptorT>
		image_patch(const std::shared_ptr<image_info> &image, InterceptorT *interceptor);

		void apply_for(const filter_t &filter);

	private:
		class patch_entry
		{
		public:
			patch_entry(symbol_info symbol, const std::shared_ptr<function_patch> &patch);

			const symbol_info &get_symbol() const;

		private:
			symbol_info _symbol;
			std::shared_ptr<function_patch> _patch;
		};

		typedef std::unordered_map<const void *, patch_entry> patches_container_t;

	private:
		const std::shared_ptr<image_info> _image;
		void * const _interceptor;
		hooks<void>::on_enter_t * const _on_enter;
		hooks<void>::on_exit_t * const _on_exit;
		patches_container_t _patches;
		executable_memory_allocator _allocator;
	};

	
	
	template <typename InterceptorT>
	inline image_patch::image_patch(const std::shared_ptr<image_info> &image, InterceptorT *interceptor)
		: _image(image), _interceptor(interceptor),
			_on_enter(hooks<InterceptorT>::on_enter()), _on_exit(hooks<InterceptorT>::on_exit())
	{	}
}
