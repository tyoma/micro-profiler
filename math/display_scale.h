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

#include <utility>

namespace math
{
	class display_scale_base
	{
	public:
		display_scale_base(unsigned samples, int pixel_size);

		std::pair<float, float> at(unsigned int index) const;

	protected:
		const float _bin_width;
	};


	class linear_display_scale : public display_scale_base
	{
	public:
		template <typename T>
		linear_display_scale(const linear_scale<T> &scale, T divisor, int pixel_size);

		float operator [](float value) const;

	private:
		const float _near, _display_k;
	};


	class log_display_scale : public display_scale_base
	{
	public:
		template <typename T>
		log_display_scale(const log_scale<T> &scale, T divisor, int pixel_size);

		float operator [](float value) const;

	private:
		const float _near, _display_k;
	};



	inline display_scale_base::display_scale_base(unsigned samples, int pixel_size)
		: _bin_width(static_cast<float>(pixel_size) / samples)
	{	}

	inline std::pair<float, float> display_scale_base::at(unsigned int index) const
	{
		float v = index * _bin_width;
		return std::make_pair(v, v + _bin_width);
	}


	template <typename T>
	inline linear_display_scale::linear_display_scale(const linear_scale<T> &scale, T divisor, int pixel_size)
		: display_scale_base(scale.samples(), pixel_size), _near(static_cast<float>(scale.near_value()) / divisor),
			_display_k((static_cast<float>(pixel_size) - _bin_width) / (static_cast<float>(scale.far_value()) / divisor - _near))
	{	}

	inline float linear_display_scale::operator [](float value) const
	{	return _display_k * (value - _near) + 0.5f * _bin_width;	}


	template <typename T>
	inline log_display_scale::log_display_scale(const log_scale<T> &scale, T divisor, int pixel_size)
		: display_scale_base(scale.samples(), pixel_size), _near(std::log10(static_cast<float>(scale.near_value()) / divisor)),
			_display_k((static_cast<float>(pixel_size) - _bin_width) / (std::log10(static_cast<float>(scale.far_value()) / divisor) - _near))
	{	}

	inline float log_display_scale::operator [](float value) const
	{	return _display_k * (std::log10(value) - _near) + 0.5f * _bin_width;	}
}
