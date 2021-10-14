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

#include <common/compiler.h>
#include <math/scale.h>

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
		float _near, _far, _major;
	};

	class linear_scale_ruler::const_iterator
	{
	public:
		const_iterator(float major_, float far, ruler_tick tick_);

		const ruler_tick &operator *() const;
		const_iterator &operator ++();

	private:
		float _major, _far;
		ruler_tick _tick;
	};



	template <typename T>
	inline linear_scale_ruler::linear_scale_ruler(const linear_scale<T> &scale, typename T divisor)
		: _near(static_cast<float>(scale.near_value()) / divisor), _far(static_cast<float>(scale.far_value()) / divisor)
	{
		const auto range_delta = _far - _near;

		_major = scale.samples() ? major_tick(range_delta) : 0.0f;
	}

	FORCE_NOINLINE float linear_scale_ruler::next_tick(float value, float ruler_tick_)
	{	return std::ceil(value / ruler_tick_) * ruler_tick_;	}

	inline float linear_scale_ruler::major_tick(float delta)
	{	return delta ? powf(10.0f, floorf(log10f(delta))) : 0.0f;	}

	inline linear_scale_ruler::const_iterator linear_scale_ruler::begin() const
	{
		if (_major)
		{
			ruler_tick t = {	_near, ruler_tick::first	};
			return const_iterator(_major, _far, t);
		}
		return end();
	}

	inline linear_scale_ruler::const_iterator linear_scale_ruler::end() const
	{
		ruler_tick t = {	_far, ruler_tick::_complete	};
		return const_iterator(_major, _far, t);
	}

	inline linear_scale_ruler::const_iterator::const_iterator(float major_, float far, ruler_tick tick_)
		: _major(major_), _far(far), _tick(tick_)
	{	}

	inline const ruler_tick &linear_scale_ruler::const_iterator::operator *() const
	{	return _tick;	}

	inline linear_scale_ruler::const_iterator &linear_scale_ruler::const_iterator::operator ++()
	{
		auto value = _tick.value;
		auto type = _tick.type;

		switch (type)
		{
		case ruler_tick::first:	value = linear_scale_ruler::next_tick(value, _major), type = ruler_tick::major;	break;
		case ruler_tick::major:	value += _major;	if (value >= _far) type = ruler_tick::last, value = _far;	break;
		case ruler_tick::last:	type = ruler_tick::_complete; break;
		}
		_tick.value = value;
		_tick.type = type;
		return *this;
	}


	inline bool operator ==(ruler_tick lhs, ruler_tick rhs)
	{	return lhs.value == rhs.value && lhs.type == rhs.type;	}

	inline bool operator ==(const linear_scale_ruler::const_iterator &lhs, const linear_scale_ruler::const_iterator &rhs)
	{	return *lhs == *rhs;	}

	inline bool operator !=(const linear_scale_ruler::const_iterator &lhs, const linear_scale_ruler::const_iterator &rhs)
	{	return !(lhs == rhs);	}
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
}
