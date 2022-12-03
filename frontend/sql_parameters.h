#pragma once

#include "constructors.h"

#include <common/formatting.h>
#include <functional>
#include <string>

namespace micro_profiler
{
	namespace sql
	{
		template <typename T>
		struct wrapped : T
		{
			wrapped(const T &inner)
				: T(inner)
			{	}
		};

		template <typename T, typename F>
		struct column_visitor
		{
			void operator ()(F T::*field_, const char *column_name_) const
			{
				if (field_ == field)
					column_name->append(column_name_);
			}

			template <typename U> 
			void operator ()(U, const char *) const
			{	}

			F T::*field;
			std::string *column_name;
		};

		template <typename T>
		struct parameter
		{
			explicit parameter(const T &object_)
				: object(object_)
			{	}

			void format(std::string &target, unsigned int &index) const
			{	target += ':', itoa<10>(target, index++);	}

			template <typename BinderT>
			void bind(const BinderT &binder) const
			{	binder(object);	}

			const T &object;
		};

		template <typename T, typename F>
		struct column
		{
			void format(std::string &target, unsigned int &/*index*/) const
			{
				auto v = initialize< column_visitor<T, F> >(field, &target);
				describe(v, static_cast<T *>(nullptr));
			}

			template <typename BinderT>
			void bind(const BinderT &/*binder*/) const
			{	}

			F T::*field;
		};

		template <typename L, typename R>
		struct operator_
		{
			void format(std::string &target, unsigned int &index) const
			{	lhs.format(target, index),	target += operator_symbol, rhs.format(target, index);	}

			template <typename BinderT>
			void bind(const BinderT &/*binder*/, unsigned int /*index*/) const
			{	}

			const L lhs;
			const R rhs;
			const char * const operator_symbol;
		};



		template <typename T>
		inline wrapped<T> wrap(const T &inner)
		{	return wrapped<T>(inner);	}

		template <typename T, typename F>
		inline wrapped< column<T, F> > c(F T::*field)
		{	return wrap(initialize< column<T, F> >(field));	}

		template <typename T>
		inline wrapped< parameter<T> > p(const T &object)
		{	return wrap(parameter<T>(object));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator ==(const wrapped<L> &lhs, const wrapped<R> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " = "));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator !=(const wrapped<L> &lhs, const wrapped<R> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " <> "));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator &&(const wrapped<L> &lhs, const wrapped<R> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " AND "));	}

		template <typename L, typename R>
		inline wrapped< operator_<L, R> > operator ||(const wrapped<L> &lhs, const wrapped<R> &rhs)
		{	return wrap(initialize< operator_<L, R> >(lhs, rhs, " OR "));	}
	}
}
