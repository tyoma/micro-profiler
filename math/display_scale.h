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

#include <utility>

namespace math
{
	class display_scale
	{
	public:
		template <typename ScaleT>
		display_scale(const ScaleT &scale_, typename ScaleT::value_type divisor, int pixel_size);

		std::pair<float, float> at(unsigned int index) const;
		float operator [](float value) const;

	private:
		float _near, _bin_width, _display_k;
	};



	template <typename S>
	inline display_scale::display_scale(const S &scale_, typename S::value_type divisor, int pixel_size_)
		: _near(static_cast<float>(scale_.near_value()) / divisor)
	{
		if (scale_.samples())
		{
			const auto pixel_size = static_cast<float>(pixel_size_);
			const auto range_delta = static_cast<float>(scale_.far_value()) / divisor - _near;

			_bin_width = pixel_size / scale_.samples();
			_display_k = (pixel_size - _bin_width) / range_delta;
		}
		else
		{
			_bin_width = 0.0f;
			_display_k = 0.0f;
		}
	}

	inline std::pair<float, float> display_scale::at(unsigned int index) const
	{
		float v = index * _bin_width;
		return std::make_pair(v, v + _bin_width);
	}

	inline float display_scale::operator [](float value) const
	{	return _display_k * (value - _near) + 0.5f * _bin_width;	}
}
