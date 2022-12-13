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

#include "async_result.h"
#include "scheduler.h"

#include <vector>

namespace scheduler
{
	template <typename T>
	class task_node;

	template <typename F, typename ArgT>
	struct result
	{	typedef decltype((*static_cast<F *>(nullptr))(*static_cast<async_result<ArgT> *>(nullptr))) type;	};


	template <typename T>
	class task : std::shared_ptr< task_node<T> >
	{
	public:
		explicit task(std::shared_ptr< task_node<T> > &&node);

		template <typename F>
		task<typename result<F, T>::type> continue_with(F &&continuation_, queue &continue_on);

		template <typename F>
		static task run(F &&callback, queue &run_on);
	};


	template <typename T>
	struct continuation
	{
		virtual void begin(const std::shared_ptr< async_result<T> > &antecedant) = 0;
	};

	template <typename T>
	class task_node
	{
	public:
		void continue_with(const std::shared_ptr< continuation<T> > &continuation_);
		void begin_continuations(const std::shared_ptr< async_result<T> > &antecedant);

	private:
		std::vector< std::shared_ptr< continuation<T> > > _continuations;
	};

	template <typename T, typename F>
	struct task_node_root : task_node<T>
	{
		task_node_root(F &&from);

		F callback;
	};

	template <typename F, typename ArgT>
	struct task_node_continuation : task_node<typename result<F, ArgT>::type>, continuation<ArgT>,
		std::enable_shared_from_this< task_node_continuation<F, ArgT> >
	{
		task_node_continuation(F &&from, queue &continue_on);

		virtual void begin(const std::shared_ptr< async_result<ArgT> > &antecedant_result) override;

	private:
		F _callback;
		queue &_continue_on;
	};



	template <typename T>
	inline task<T>::task(std::shared_ptr< task_node<T> > &&node)
		: std::shared_ptr< task_node<T> >(std::forward< std::shared_ptr< task_node<T> > >(node))
	{	}

	template <typename T>
	template <typename F>
	inline task<typename result<F, T>::type> task<T>::continue_with(F &&continuation_, queue &continue_on)
	{
		auto c = std::make_shared< task_node_continuation<F, T> >(std::forward<F>(continuation_), continue_on);

		(*this)->continue_with(c);
		return task<typename result<F, T>::type>(std::move(c));
	}

	template <typename T>
	template <typename F>
	inline task<T> task<T>::run(F &&callback, queue &run_on)
	{
		auto r = std::make_shared< task_node_root<T, F> >(std::forward<F>(callback));

		run_on.schedule([r] {
			auto result = std::make_shared< async_result<T> >();

			result->set(r->callback());
			r->begin_continuations(result);
		});
		return task(std::move(r));
	}


	template <typename T>
	inline void task_node<T>::continue_with(const std::shared_ptr< continuation<T> > &continuation_)
	{
		_continuations.push_back(continuation_);
	}

	template <typename T>
	inline void task_node<T>::begin_continuations(const std::shared_ptr< async_result<T> > &antecedant)
	{
		for (auto i = std::begin(_continuations); i != std::end(_continuations); ++i)
			(*i)->begin(antecedant);
	}


	template <typename T, typename F>
	inline task_node_root<T, F>::task_node_root(F &&from)
		: callback(std::forward<F>(from))
	{	}


	template <typename F, typename ArgT>
	inline task_node_continuation<F, ArgT>::task_node_continuation(F &&from, queue &continue_on)
		: _callback(std::forward<F>(from)), _continue_on(continue_on)
	{	}

	template <typename F, typename ArgT>
	inline void task_node_continuation<F, ArgT>::begin(const std::shared_ptr< async_result<ArgT> > &antecedant_result)
	{
		auto self = this->shared_from_this();

		_continue_on.schedule([antecedant_result, self] {
			auto result_ = std::make_shared< async_result<typename result<F, ArgT>::type> >();

			result_->set(self->_callback(*antecedant_result));
			self->begin_continuations(result_);
		});
	}
}
