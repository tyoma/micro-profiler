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

#include <stdexcept>
#include <tuple>
#include <vector>

namespace scheduler
{
	struct queue;

	template <typename F, typename ArgT>
	struct result
	{	typedef decltype((*static_cast<F *>(nullptr))(*static_cast<ArgT *>(nullptr))) type;	};

	template <typename T>
	class async_result
	{
	public:
		async_result()
			: _state(empty)
		{	}

		~async_result()
		{
			if (has_value == _state)
				static_cast<T *>(static_cast<void *>(_buffer))->~T();
			else if (has_exception == _state)
				static_cast<std::exception_ptr *>(static_cast<void *>(_buffer))->~exception_ptr();
		}

		void set(T &&from)
		{
			check_set();
			new (_buffer) T(std::forward<T>(from));
			_state = has_value;
		}

		void fail(std::exception_ptr &&exception)
		{
			check_set();
			new (_buffer) std::exception_ptr(std::forward<std::exception_ptr>(exception));
			_state = has_exception;
		}

		const T &operator *() const
		{
			check_read();
			return *static_cast<const T *>(static_cast<const void *>(_buffer));
		}

	protected:
		void check_set() const
		{
			if (empty != _state)
				throw std::logic_error("a value/exception has already been set");
		}

		void check_read() const
		{
			if (has_exception == _state)
				rethrow_exception(*static_cast<const std::exception_ptr *>(static_cast<const void *>(_buffer)));
		}

	private:
		async_result(const async_result &other);
		void operator =(const async_result &rhs);

	private:
		char _buffer[sizeof(T) > sizeof(std::exception_ptr) ? sizeof(T) : sizeof(std::exception_ptr)];

	protected:
		enum {	empty, has_value, has_exception	} _state : 8;
	};

	template <>
	class async_result<void> : async_result<int>
	{
	public:
		void set()
		{	
			check_set();
			_state = has_value;
		}

		using async_result<int>::fail;

		void operator *() const
		{	check_read();	}
	};


	template <typename T>
	struct continuation
	{
		virtual void begin(const std::shared_ptr< async_result<T> > &antecedant) = 0;
	};


	template <typename T>
	class task
	{
	public:
		template <typename F>
		task<typename result< F, async_result<T> >::type> continue_with(F &&continuation_, queue &continue_on)
		{
			typedef typename result< F, async_result<T> >::type descendant_type;
			typedef task<descendant_type> descendant_task;
			typedef typename descendant_task::template core_continuation<F, T> continuation_task_type;

			auto c = std::make_shared<continuation_task_type>(std::forward<F>(continuation_), continue_on);

			_core->add_continuation(c);
			return descendant_task(c);
		}

		const T &operator *() const
		{	return *_core->result;	}

		template <typename F>
		static task run(F &&callback, queue &run_on)
		{
			auto c = std::make_shared< core_root<F> >(std::forward<F>(callback));

			run_on.schedule([c] {
				auto result = std::make_shared< async_result<T> >();

				result->set(c->callback());
				c->begin_continuations(result);
			});
			return task(c);
		}

	private:
		struct core
		{
			void add_continuation(const std::shared_ptr< continuation<T> > &descendant)
			{
				_continuations.push_back(descendant);
			}

			void begin_continuations(const std::shared_ptr< async_result<T> > &antecedant)
			{
				for (auto i = std::begin(_continuations); i != std::end(_continuations); ++i)
					(*i)->begin(antecedant);
			}

		private:
			std::vector< std::shared_ptr< continuation<T> > > _continuations;
		};

		template <typename F>
		struct core_root : core
		{
			core_root(F &&from)
				: callback(std::forward<F>(from))
			{	}

			F callback;
		};

		template <typename F, typename ArgT>
		struct core_continuation : core, continuation<ArgT>, std::enable_shared_from_this< core_continuation<F, ArgT> >
		{
			core_continuation(F &&from, queue &continue_on)
				: _callback(std::forward<F>(from)), _continue_on(continue_on)
			{	}

			virtual void begin(const std::shared_ptr< async_result<ArgT> > &antecedant_result) override
			{
				auto self = this->shared_from_this();

				_continue_on.schedule([antecedant_result, self] {
					auto result = std::make_shared< async_result<T> >();

					result->set(self->_callback(*antecedant_result));
					self->begin_continuations(result);
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
