//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "scheduler.h"

#include <functional>
#include <tuple>
#include <vector>

namespace scheduler
{
	struct queue;

	template <typename F, typename ArgT>
	struct result
	{	typedef decltype((*static_cast<F *>(nullptr))(*static_cast<ArgT *>(nullptr))) type;	};

	template <typename T>
	class task
	{
	public:
		template <typename F>
		task<typename result<F, task>::type> continue_with(F &&continuation_, queue &continue_on)
		{
			typedef task<typename result<F, task>::type> descendant_task;
			typedef typename descendant_task::template core_continuation<F, task> continuation_task_type;

			auto c = std::make_shared<continuation_task_type>(std::forward<F>(continuation_), continue_on);

			_core->add_continuation(c);
			return descendant_task(c);
		}

		const T &operator *() const
		{	return *_core->value;	}

		template <typename F>
		static task run(F &&callback, queue &run_on)
		{
			auto c = std::make_shared< core_root<F> >(std::forward<F>(callback));

			run_on.schedule([c] {
				c->value.reset(new T(c->callback()));
				c->schedule_continuations(task<T>(c));
			});
			return task(c);
		}

	private:
		struct continuation
		{
			virtual void schedule(const task &antecedant) = 0;
		};

		struct core
		{
			void add_continuation(const std::shared_ptr<continuation> &descendant)
			{
				_continuations.push_back(descendant);
			}

			void schedule_continuations(const task &antecedant)
			{
				for (auto i = std::begin(_continuations); i != std::end(_continuations); ++i)
					(*i)->schedule(antecedant);
			}

			std::unique_ptr<T> value;

		private:
			std::vector< std::shared_ptr<continuation> > _continuations;
		};

		template <typename F>
		struct core_root : core
		{
			core_root(F &&from)
				: callback(std::forward<F>(from))
			{	}

			F callback;
		};

		template <typename F, typename SourceTaskT>
		struct core_continuation : core, SourceTaskT::continuation,
			std::enable_shared_from_this< core_continuation<F, SourceTaskT> >
		{
			core_continuation(F &&from, queue &continue_on)
				: _callback(std::forward<F>(from)), _continue_on(continue_on)
			{	}

			virtual void schedule(const SourceTaskT &antecedant) override
			{
				auto self = this->shared_from_this();

				_continue_on.schedule([antecedant, self] {
					self->value.reset(new T(self->_callback(antecedant)));
					self->schedule_continuations(task<T>(self));
				});
			}

		private:
			F _callback;
			queue &_continue_on;
		};

	private:
		explicit task(const std::shared_ptr<core> &core_)
			:_core(core_)
		{	}

	private:
		std::shared_ptr<core> _core;

	private:
		template <typename U>
		friend class task;
	};
}
