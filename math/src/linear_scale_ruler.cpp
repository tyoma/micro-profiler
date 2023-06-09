//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <common/compiler.h>

using namespace std;

namespace math
{
	FORCE_NOINLINE float linear_scale_ruler::next_tick(float value, float ruler_tick_)
	{	return ceil(value / ruler_tick_) * ruler_tick_;	}

	float linear_scale_ruler::major_tick(float delta)
	{	return delta ? pow(10.0f, floor(log10(delta))) : 0.0f;	}

	linear_scale_ruler::const_iterator linear_scale_ruler::begin() const
	{
		if (_major)
		{
			ruler_tick t = {	_near, ruler_tick::first	};
			return const_iterator(_major, _far, t);
		}
		return end();
	}

	linear_scale_ruler::const_iterator linear_scale_ruler::end() const
	{
		ruler_tick t = {	_far, ruler_tick::_complete	};
		return const_iterator(_major, _far, t);
	}


	linear_scale_ruler::const_iterator::const_iterator(float major_, float far, ruler_tick tick_)
		: _major(major_), _far(far), _tick(tick_)
	{	}

	const ruler_tick &linear_scale_ruler::const_iterator::operator *() const
	{	return _tick;	}

	linear_scale_ruler::const_iterator &linear_scale_ruler::const_iterator::operator ++()
	{
		auto value = _tick.value;
		auto type = _tick.type;

		switch (type)
		{
		case ruler_tick::first:
			value = linear_scale_ruler::next_tick(value, _major);
			type = ruler_tick::major;
			break;

		case ruler_tick::major:
			value += _major;
			if (value >= _far)
				type = ruler_tick::last, value = _far;
			break;

		case ruler_tick::last:
			type = ruler_tick::_complete;
			break;
		}
		_tick.value = value;
		_tick.type = type;
		return *this;
	}
}
