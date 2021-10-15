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

#include <math/scale_ruler.h>

using namespace std;

namespace math
{
	log_scale_ruler::const_iterator log_scale_ruler::begin() const
	{
		ruler_tick t = {	_near, ruler_tick::first	};
		return const_iterator(_far, t);
	}

	log_scale_ruler::const_iterator log_scale_ruler::end() const
	{
		ruler_tick t = {	_far, ruler_tick::_complete	};
		return const_iterator(_far, t);
	}


	log_scale_ruler::const_iterator::const_iterator(float far_, ruler_tick tick_)
		: _far(far_), _tick(tick_), _index(tick_index::next(tick_.value))
	{	}

	const ruler_tick &log_scale_ruler::const_iterator::operator *() const
	{	return _tick;	}

	log_scale_ruler::const_iterator &log_scale_ruler::const_iterator::operator ++()
	{
		auto value = _tick.value;
		auto type = _tick.type;

		switch (type)
		{
		case ruler_tick::minor:
		case ruler_tick::major:
			++_index;

		case ruler_tick::first:
			value = _index.value();
			if (value >= _far)
				value = _far, type = ruler_tick::last;
			else
				type = _index.minor == 1u ? ruler_tick::major : ruler_tick::minor;
			break;

		case ruler_tick::last:
			type = ruler_tick::_complete;
			break;
		}
		_tick.value = value;
		_tick.type = type;
		return *this;
	}


	log_scale_ruler::tick_index log_scale_ruler::tick_index::next(float value)
	{
		const auto order = floor(log10(value));
		log_scale_ruler::tick_index index = {
			static_cast<signed char>(order),
			static_cast<unsigned char>(floor(value / pow(10.0f, order)))
		};

		return ++index;
	}

	float log_scale_ruler::tick_index::value() const
	{	return minor * pow(10.0f, static_cast<float>(order));	}

	log_scale_ruler::tick_index &log_scale_ruler::tick_index::operator ++()
	{
		if (++minor == 10u)
			order++, minor = 1;
		return *this;
	}
}
