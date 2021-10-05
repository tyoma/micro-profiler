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

#include <vector>

namespace math
{
	class histogram : std::vector<value_t>
	{
	public:
		typedef std::vector<value_t> base_t;

	public:
		void set_scale(const scale &scale_);
		const scale &get_scale() const;

		using base_t::size;
		using base_t::begin;
		using base_t::end;

		void add(value_t value, value_t d = 1);

		void reset();

	private:
		scale _scale;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, histogram &data, unsigned int ver);
	};



	inline const scale &histogram::get_scale() const
	{	return _scale;	}

	inline void histogram::set_scale(const scale &scale_)
	{
		assign(scale_.samples(), value_t());
		_scale = scale_;
	}

	inline void histogram::add(value_t at, value_t d)
	{
		index_t index = 0;

		if (_scale(index, at))
			(*this)[index] += d;
	}

	inline void histogram::reset()
	{	assign(size(), value_t());	}


	inline histogram &operator +=(histogram &lhs, const histogram &rhs)
	{
		if (lhs.get_scale() != rhs.get_scale())
		{
			lhs = rhs;
		}
		else
		{
			auto l = lhs.begin();
			auto r = rhs.begin();

			for (auto e = lhs.end(); l != e; ++l, ++r)
				*l += *r;
		}
		return lhs;
	}

	inline void interpolate(histogram &lhs, const histogram &rhs, float alpha)
	{
		if (lhs.get_scale() != rhs.get_scale())
			lhs.set_scale(rhs.get_scale());

		auto l = lhs.begin();
		auto r = rhs.begin();
		const auto ialpha = static_cast<int>(256.0f * alpha);

		for (auto e = lhs.end(); l != e; ++l, ++r)
			*l += (*r - *l) * ialpha >> 8;
	}
}
