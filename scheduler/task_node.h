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

#include <memory>
#include <mt/mutex.h>
#include <vector>

namespace scheduler
{
	template <typename T>
	struct continuation
	{
		typedef std::shared_ptr<continuation> ptr;

		virtual void begin(const std::shared_ptr< const async_result<T> > &antecedant) = 0;
	};

	template <typename T>
	class task_node_base
	{
	public:
		typedef typename continuation<T>::ptr continuation_ptr;

	public:
		void continue_with(const continuation_ptr &continuation_);

		void fail(std::exception_ptr &&exception);

	protected:
		void set_result(std::shared_ptr< const async_result<T> > &&result);

	private:
		mt::mutex _mtx;
		std::shared_ptr< const async_result<T> > _result;
		std::vector<continuation_ptr> _continuations;
	};

	template <typename T>
	struct task_node : public task_node_base<T>
	{
		void set(T &&result);
	};

	template <>
	struct task_node<void> : public task_node_base<void>
	{
		void set();
	};



	template <typename T>
	inline void task_node_base<T>::continue_with(const continuation_ptr &continuation_)
	{
		{
			mt::lock_guard<mt::mutex> l(_mtx);

			if (!_result)
			{
				_continuations.push_back(continuation_);
				return;
			}
		}
		continuation_->begin(_result);
	}

	template <typename T>
	inline void task_node_base<T>::fail(std::exception_ptr &&exception)
	{
		auto aresult = std::make_shared< async_result<T> >();

		aresult->fail(std::forward<std::exception_ptr>(exception));
		set_result(std::move(aresult));
	}

	template <typename T>
	inline void task_node_base<T>::set_result(std::shared_ptr< const async_result<T> > &&result)
	{
		std::vector<continuation_ptr> continuations;

		{
			mt::lock_guard<mt::mutex> l(_mtx);

			_result = std::move(result);
			continuations.swap(_continuations);
		}
		for (auto i = std::begin(continuations); i != std::end(continuations); ++i)
			(*i)->begin(_result);
	}


	template <typename T>
	inline void task_node<T>::set(T &&result)
	{
		auto aresult = std::make_shared< async_result<T> >();

		aresult->set(std::forward<T>(result));
		this->set_result(std::move(aresult));
	}


	inline void task_node<void>::set()
	{
		auto aresult = std::make_shared< async_result<void> >();

		aresult->set();
		this->set_result(std::move(aresult));
	}
}
