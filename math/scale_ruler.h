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

#include <iterator>

namespace math
{
	struct ruler_tick
	{
		enum types {	first, last, major, minor, _complete /*never exposed*/,	};

		float value;
		types type;
	};


	class linear_scale_ruler
	{
	public:
		typedef ruler_tick value_type;
		class const_iterator;

	public:
		template <typename T>
		linear_scale_ruler(const linear_scale<T> &scale, T divisor);

		static float next_tick(float value, float ruler_tick_);
		static float major_tick(float delta);

		const_iterator begin() const;
		const_iterator end() const;

	private:
		const float _near, _far, _major;
	};

	class linear_scale_ruler::const_iterator
	{
	public:
		const_iterator(float major_, float far, ruler_tick tick_);

		const ruler_tick &operator *() const;
		const_iterator &operator ++();

	private:
		const float _major, _far;
		ruler_tick _tick;
	};


	class log_scale_ruler
	{
	public:
		struct tick_index;
		typedef ruler_tick value_type;
		class const_iterator;

	public:
		template <typename T>
		log_scale_ruler(const log_scale<T> &scale, T divisor);

		const_iterator begin() const;
		const_iterator end() const;

	private:
		const float _near, _far;
	};

	struct log_scale_ruler::tick_index
	{
		static tick_index next(float value);

		float value() const;
		tick_index &operator ++();

		signed char order;
		unsigned char minor;
	};

	class log_scale_ruler::const_iterator
	{
	public:
		const_iterator(float far, ruler_tick tick_);

		const ruler_tick &operator *() const;
		const_iterator &operator ++();

	private:
		const float _far;
		ruler_tick _tick;
		tick_index _index;
	};



	template <typename T>
	inline linear_scale_ruler::linear_scale_ruler(const linear_scale<T> &scale, T divisor)
		: _near(static_cast<float>(scale.near_value()) / divisor), _far(static_cast<float>(scale.far_value()) / divisor),
			_major(major_tick(_far - _near))
	{	}


	template <typename T>
	inline log_scale_ruler::log_scale_ruler(const log_scale<T> &scale, T divisor)
		: _near(static_cast<float>(scale.near_value()) / divisor), _far(static_cast<float>(scale.far_value()) / divisor)
	{	}


	inline bool operator ==(ruler_tick lhs, ruler_tick rhs)
	{	return (lhs.value == rhs.value) & (lhs.type == rhs.type);	}

	inline bool operator ==(const linear_scale_ruler::const_iterator &lhs, const linear_scale_ruler::const_iterator &rhs)
	{	return *lhs == *rhs;	}

	inline bool operator !=(const linear_scale_ruler::const_iterator &lhs, const linear_scale_ruler::const_iterator &rhs)
	{	return !(lhs == rhs);	}

	inline bool operator ==(const log_scale_ruler::const_iterator &lhs, const log_scale_ruler::const_iterator &rhs)
	{	return *lhs == *rhs;	}

	inline bool operator !=(const log_scale_ruler::const_iterator &lhs, const log_scale_ruler::const_iterator &rhs)
	{	return !(lhs == rhs);	}

	inline bool operator ==(log_scale_ruler::tick_index lhs, log_scale_ruler::tick_index rhs)
	{	return (lhs.order == rhs.order) & (lhs.minor == rhs.minor);	}
}

namespace std
{
	template<>
	struct iterator_traits<math::linear_scale_ruler::const_iterator>
	{
		typedef forward_iterator_tag iterator_category;
		typedef math::ruler_tick value_type;
		typedef int difference_type;
		typedef value_type reference;
	};

	template<>
	struct iterator_traits<math::log_scale_ruler::const_iterator>
	{
		typedef forward_iterator_tag iterator_category;
		typedef math::ruler_tick value_type;
		typedef int difference_type;
		typedef value_type reference;
	};
}
