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

#include <common/histogram.h>

namespace micro_profiler
{
	scale::scale(value_t near, value_t far, unsigned int samples_)
		: _samples(samples_), _near(near), _far(far)
	{	reset();	}

	void scale::reset()
	{
		if (_samples > 1)
		{
			_scale = static_cast<float>(_far - _near) / (_samples - 1);
			_base = _near - static_cast<value_t>(0.5f * _scale);
			_scale = 1.0f / _scale;
		}
		else
		{
			_base = 0;
			_scale = 0.0f;
		}
	}


	void histogram::set_scale(const scale &scale_)
	{
		assign(scale_.samples(), value_t());
		_scale = scale_;
	}

	void histogram::reset()
	{	assign(size(), value_t());	}

	histogram &histogram::operator +=(const histogram &rhs)
	{
		if (_scale != rhs.get_scale())
		{
			*this = rhs;
		}
		else
		{
			auto l = begin();
			auto r = rhs.begin();

			for (; l != end(); ++l, ++r)
				*l += *r;
		}
		return *this;
	}
}
