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

#include <cmath>
#include <common/compiler.h>
#include <iterator>
#include <utility>

namespace math
{
	class display_scale
	{
	public:
		enum tick_type {	first, last, major, minor, _complete /*never exposed*/,	};
		struct tick;
		typedef tick value_type;
		class const_iterator;

	public:
		template <typename ScaleT>
		display_scale(const ScaleT &scale_, typename ScaleT::value_type divisor, int pixel_size);

		static float next_tick(float value, float tick);
		static float major_tick(float delta);

		const_iterator begin() const;
		const_iterator end() const;
		std::pair<float, float> at(unsigned int index) const;
		float operator [](float value) const;

	private:
		float _near, _far, _major, _bin_width, _display_k;
	};

	struct display_scale::tick
	{
		float value;
		tick_type type;
	};

	class display_scale::const_iterator
	{
	public:
		const_iterator(float major_, float far, tick tick_);

		const tick &operator *() const;
		const_iterator &operator ++();

	private:
		float _major, _far;
		tick _tick;
	};



	template <typename S>
	inline display_scale::display_scale(const S &scale_, typename S::value_type divisor, int pixel_size_)
		: _near(static_cast<float>(scale_.near_value()) / divisor), _far(static_cast<float>(scale_.far_value()) / divisor)
	{
		if (scale_.samples())
		{
			const auto pixel_size = static_cast<float>(pixel_size_);
			const auto range_delta = _far - _near;

			_major = major_tick(range_delta);
			_bin_width = pixel_size / scale_.samples();
			_display_k = (pixel_size - _bin_width) / range_delta;
		}
		else
		{
			_major = 0.0f;
			_bin_width = 0.0f;
			_display_k = 0.0f;
		}
	}

	FORCE_NOINLINE float display_scale::next_tick(float value, float tick)
	{	return std::ceil(value / tick) * tick;	}

	inline float display_scale::major_tick(float delta)
	{	return delta ? powf(10.0f, floorf(log10f(delta))) : 0.0f;	}

	inline display_scale::const_iterator display_scale::begin() const
	{
		if (_major)
		{
			tick t = {	_near, first	};
			return const_iterator(_major, _far, t);
		}
		return end();
	}

	inline display_scale::const_iterator display_scale::end() const
	{
		tick t = {	_far, _complete	};
		return const_iterator(_major, _far, t);
	}

	inline std::pair<float, float> display_scale::at(unsigned int index) const
	{
		float v = index * _bin_width;
		return std::make_pair(v, v + _bin_width);
	}

	inline float display_scale::operator [](float value) const
	{	return _display_k * (value - _near) + 0.5f * _bin_width;	}


	inline display_scale::const_iterator::const_iterator(float major_, float far, tick tick_)
		: _major(major_), _far(far), _tick(tick_)
	{	}

	inline const display_scale::tick &display_scale::const_iterator::operator *() const
	{	return _tick;	}

	inline display_scale::const_iterator &display_scale::const_iterator::operator ++()
	{
		auto value = _tick.value;
		auto type = _tick.type;

		switch (type)
		{
		case first:	value = display_scale::next_tick(value, _major), type = major;	break;
		case major:	value += _major;	if (value >= _far) type = last, value = _far;	break;
		case last:	type = _complete; break;
		}
		_tick.value = value;
		_tick.type = type;
		return *this;
	}


	inline bool operator ==(display_scale::tick lhs, display_scale::tick rhs)
	{	return lhs.value == rhs.value && lhs.type == rhs.type;	}

	inline bool operator ==(const display_scale::const_iterator &lhs, const display_scale::const_iterator &rhs)
	{	return *lhs == *rhs;	}

	inline bool operator !=(const display_scale::const_iterator &lhs, const display_scale::const_iterator &rhs)
	{	return !(lhs == rhs);	}
}

namespace std
{
	template<>
	struct iterator_traits<math::display_scale::const_iterator>
	{
		typedef forward_iterator_tag iterator_category;
		typedef math::display_scale::tick value_type;
		typedef int difference_type;
		typedef value_type reference;
	};
}
