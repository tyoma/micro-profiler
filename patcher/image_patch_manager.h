//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "function_patch.h"
#include "interface.h"

#include <common/noncopyable.h>
#include <common/range.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace micro_profiler
{
	class executable_memory_allocator;

	class image_patch_manager : public patch_manager, noncopyable
	{
	public:
		template <typename InterceptorT>
		image_patch_manager(InterceptorT &interceptor, executable_memory_allocator &allocator_);

		void detach_all();

		virtual void query(patch_state &states, unsigned int persistent_id) override;
		virtual void apply(apply_results &results, unsigned int persistent_id, void *base, std::shared_ptr<void> lock,
			request_range targets) override;
		virtual void revert(revert_results &results, unsigned int persistent_id, request_range targets) override;

	private:
		struct image_patch
		{
			image_patch();

			std::shared_ptr<void> lock;
			std::unordered_map< unsigned int /*rva*/, std::shared_ptr<function_patch> > patched;
			unsigned int patches_applied;
		};

		typedef std::unordered_map<unsigned int /*persistent_id*/, image_patch> patched_images_t;

	private:
		patched_images_t _patched_images;
		std::function<std::shared_ptr<function_patch> (void *target)> _create_patch;
	};



	template <typename InterceptorT>
	inline std::shared_ptr<function_patch> construct_function_patch(void *target, InterceptorT &interceptor, executable_memory_allocator &allocator_)
	{	return std::make_shared<function_patch>(target, &interceptor, allocator_);	}

	template <typename InterceptorT>
	inline image_patch_manager::image_patch_manager(InterceptorT &interceptor, executable_memory_allocator &allocator_)
		: _create_patch([&interceptor, &allocator_] (void *target) {
			return construct_function_patch(target, interceptor, allocator_);
		})
	{	}
}
