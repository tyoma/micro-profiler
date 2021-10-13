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

#include "scale.h"
#include "variant.h"

namespace math
{
	template <typename T>
	class variant_scale : public variant< log_scale<T>, variant< linear_scale<T> > >
	{
	public:
		typedef variant< log_scale<T>, variant< linear_scale<T> > > base_t;
		typedef T value_type;

	public:
		variant_scale();
		template <typename U>
		variant_scale(const U &underlying);

		value_type near_value() const;
		value_type far_value() const;
		index_t samples() const;
		bool operator ()(index_t &index, value_type value) const;

	private:
		struct get_near : static_visitor<value_type>
		{	template <typename U> value_type operator ()(const U &u) const {	return u.near_value();	}	};

		struct get_far : static_visitor<value_type>
		{	template <typename U> value_type operator ()(const U &u) const {	return u.far_value();	}	};

		struct get_samples : static_visitor<index_t>
		{	template <typename U> index_t operator ()(const U &u) const {	return u.samples();	}	};

		struct convert
		{
			typedef bool return_type;

			template <typename U> bool operator ()(const U &u) const {	return u(index, value);	}

			index_t &index;
			value_type value;
		};

	};



	template <typename T>
	inline variant_scale<T>::variant_scale()
		: base_t(linear_scale<T>())
	{	}

	template <typename T>
	template <typename U>
	inline variant_scale<T>::variant_scale(const U &underlying)
		: base_t(underlying)
	{	}

	template <typename T>
	inline T variant_scale<T>::near_value() const
	{	return base_t::visit(get_near());	}

	template <typename T>
	inline T variant_scale<T>::far_value() const
	{	return base_t::visit(get_far());	}

	template <typename T>
	inline index_t variant_scale<T>::samples() const
	{	return base_t::visit(get_samples());	}

	template <typename T>
	inline bool variant_scale<T>::operator ()(index_t &index, value_type value) const
	{
		convert v = {	index, value	};
		return base_t::visit(v);
	}


	template <typename T>
	inline bool operator ==(const variant_scale<T> &lhs, const variant_scale<T> &rhs)
	{
		return static_cast<const typename variant_scale<T>::base_t &>(lhs)
			== static_cast<const typename variant_scale<T>::base_t &>(rhs);
	}

	template <typename T>
	inline bool operator !=(const variant_scale<T> &lhs, const variant_scale<T> &rhs)
	{	return !(lhs == rhs);	}
}
