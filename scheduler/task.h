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
#include "task_node.h"

namespace scheduler
{
	template <typename T>
	class task;


	template <typename F, typename ArgT>
	struct task_result
	{	typedef decltype((*static_cast<F *>(nullptr))(*static_cast<async_result<ArgT> *>(nullptr))) type;	};

	template <typename T>
	struct unwrapped_result
	{	typedef void type;	};

	template <typename T>
	struct unwrapped_result< task<T> >
	{	typedef T type;	};


	template <typename T>
	class task : std::shared_ptr< task_node<T> >
	{
	public:
		explicit task(std::shared_ptr< task_node<T> > &&node);

		template <typename F>
		task<typename task_result<F, T>::type> continue_with(F &&continuation_callback, queue &continue_on);

		task<typename unwrapped_result<T>::type> unwrap();

		template <typename F>
		static task run(F &&task_callback, queue &run_on);

	private:
		template <typename T2>
		friend struct task_unwrap;
	};


	template <typename T, typename F>
	struct task_root : task_node<T>
	{
		task_root(F &&from);

		F callback;
	};

	template <typename F, typename ArgT>
	struct task_continuation : task_node<typename task_result<F, ArgT>::type>, continuation<ArgT>
	{
		task_continuation(F &&from, queue &continue_on);

		virtual void begin(const std::shared_ptr< const async_result<ArgT> > &antecedant_result) override;

	private:
		F _callback;
		queue &_continue_on;
	};

	template <typename T>
	struct task_unwrap : task_node<T>, continuation< task<T> >, continuation<T>
	{
		virtual void begin(const std::shared_ptr< const async_result< task<T> > > &antecedant_result) override
		{	(**antecedant_result)->continue_with(std::static_pointer_cast<task_unwrap>(this->shared_from_this()));	}

		virtual void begin(const std::shared_ptr< const async_result<T> > &antecedant_result) override
		{	this->set_result([&] (async_result<T> &r) {	r = *antecedant_result;	});	}
	};



	template <typename T, typename F>
	inline void handle_exception(task_node<T> &target, F &&f)
	{
		try
		{	f();	}
		catch (...)
		{	target.fail(std::current_exception());	}
	}

	template <typename T, typename CallbackT>
	inline void set_result(task_node<T> &target, CallbackT &callback)
	{	handle_exception(target, [&] {	target.set(callback());	});	}

	template <typename CallbackT>
	inline void set_result(task_node<void> &target, CallbackT &callback)
	{	handle_exception(target, [&] {	callback(), target.set();	});	}

	template <typename T, typename CallbackT, typename A>
	inline void set_result(task_node<T> &target, CallbackT &callback, A &antecedent)
	{	handle_exception(target, [&] {	target.set(callback(antecedent));	});	}

	template <typename CallbackT, typename A>
	inline void set_result(task_node<void> &target, CallbackT &callback, A &antecedent)
	{	handle_exception(target, [&] {	callback(antecedent), target.set();	});	}


	template <typename T>
	inline task<T>::task(std::shared_ptr< task_node<T> > &&node)
		: std::shared_ptr< task_node<T> >(std::forward< std::shared_ptr< task_node<T> > >(node))
	{	}

	template <typename T>
	template <typename F>
	inline task<typename task_result<F, T>::type> task<T>::continue_with(F &&continuation_callback, queue &continue_on)
	{
		auto c = std::make_shared< task_continuation<F, T> >(std::forward<F>(continuation_callback), continue_on);

		(*this)->continue_with(c);
		return task<typename task_result<F, T>::type>(std::move(c));
	}

	template <typename T>
	inline task<typename unwrapped_result<T>::type> task<T>::unwrap()
	{
		auto c = std::make_shared< task_unwrap<typename unwrapped_result<T>::type> >();

		(*this)->continue_with(c);
		return task<typename unwrapped_result<T>::type>(std::move(c));
	}

	template <typename T>
	template <typename F>
	inline task<T> task<T>::run(F &&task_callback, queue &run_on)
	{
		auto r = std::make_shared< task_root<T, F> >(std::forward<F>(task_callback));

		run_on.schedule([r] {	scheduler::set_result(*r, r->callback);	});
		return task(std::move(r));
	}


	template <typename T, typename F>
	inline task_root<T, F>::task_root(F &&from)
		: callback(std::forward<F>(from))
	{	}


	template <typename F, typename ArgT>
	inline task_continuation<F, ArgT>::task_continuation(F &&from, queue &continue_on)
		: _callback(std::forward<F>(from)), _continue_on(continue_on)
	{	}

	template <typename F, typename ArgT>
	inline void task_continuation<F, ArgT>::begin(const std::shared_ptr< const async_result<ArgT> > &result)
	{
		auto self = std::static_pointer_cast<task_continuation>(this->shared_from_this());

		_continue_on.schedule([result, self] {	scheduler::set_result(*self, self->_callback, *result);	});
	}
}
