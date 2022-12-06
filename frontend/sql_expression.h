#pragma once

#include "constructors.h"

#include <type_traits>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T, typename R = typename T::result_type>
		struct wrapped : T
		{
			wrapped(const T &inner)
				: T(inner)
			{	}
		};

		template <typename T>
		struct parameter
		{
			typedef typename std::remove_cv<T>::type result_type;

			T &object;
		};

		template <typename T, typename F>
		struct column
		{
			typedef F result_type;

			F T::*field;
		};

		template <typename L, typename R>
		struct operator_
		{
			typedef bool result_type;

			L lhs;
			R rhs;
			const char *literal;
		};



		template <typename T>
		inline wrapped<T> wrap(const T &inner)
		{	return wrapped<T>(inner);	}

		template <typename T, typename F>
		inline wrapped< column<T, F> > c(F T::*field)
		{	return wrap(initialize< column<T, F> >(field));	}

		template <typename T>
		inline wrapped< parameter<T> > p(T &object)
		{
			parameter<T> param = {	object	};
			return wrap(param);
		}

		template <typename L, typename R, typename T>
		inline wrapped< operator_<L, R> > operator ==(const wrapped<L, T> &lhs, const wrapped<R, T> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, "="));	}

		template <typename L, typename R, typename T>
		inline wrapped< operator_<L, R> > operator !=(const wrapped<L, T> &lhs, const wrapped<R, T> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, "<>"));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator &&(const wrapped<L, bool> &lhs, const wrapped<R, bool> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " AND "));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator ||(const wrapped<L, bool> &lhs, const wrapped<R, bool> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " OR "));	}

		template <typename T, typename VisitorT>
		inline void describe(VisitorT &&visitor)
		{	describe(visitor, static_cast<T *>(nullptr));	}
	}
}
