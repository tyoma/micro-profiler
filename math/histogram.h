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

#include <algorithm>
#include <vector>

namespace math
{
	typedef unsigned int index_t;

	template <typename ScaleT, typename Y>
	class histogram : std::vector<Y>
	{
	public:
		typedef std::vector<Y> base_t;
		typedef ScaleT scale_type;
		typedef typename scale_type::value_type x_type;
		typedef Y y_type;

	public:
		void set_scale(const scale_type &scale_);
		const scale_type &get_scale() const;

		using base_t::size;
		using base_t::begin;
		using base_t::end;

		void add(x_type value, y_type d = 1);

		template <typename IteratorT /*partition*/>
		void find_partitions(IteratorT first, IteratorT last) const;

		void reset();

	private:
		scale_type _scale;

	private:
		template <typename ArchiveT, typename ScaleT_, typename Y_>
		friend void serialize(ArchiveT &archive, histogram<ScaleT_, Y_> &data, unsigned int ver);
	};

	struct partition_less
	{
		template <typename T>
		bool operator ()(const T &lhs, const T &rhs) const
		{	return lhs.midvalue < rhs.midvalue;	}
	};




	template <typename S, typename Y>
	inline const S &histogram<S, Y>::get_scale() const
	{	return _scale;	}

	template <typename S, typename Y>
	inline void histogram<S, Y>::set_scale(const scale_type &scale_)
	{
		base_t::assign(scale_.samples(), y_type());
		_scale = scale_;
	}

	template <typename S, typename Y>
	inline void histogram<S, Y>::add(x_type at, y_type d)
	{
		index_t index = 0;

		if (_scale(index, at))
			(*this)[index] += d;
	}

	template <typename S, typename Y>
	template <typename I>
	inline void histogram<S, Y>::find_partitions(I first, I last) const
	{
		std::sort(first, last, partition_less());

		const auto n = _scale.samples();
		auto j = n - n;
		auto sum = Y();

		for (; j + 1 < n; ++j)
		{
			sum = (*this)[j];
			if (sum)
				break;
		}

		for (; first != last && first->midvalue <= sum; ++first)
			first->location = _scale[j];

		for (index_t i = j + 1; i < n; ++i)
		{
			auto d = (*this)[i];
			auto new_sum = sum + d;

			for (; first != last && first->midvalue <= new_sum; ++first)
			{
				const auto previous = _scale[j];

				first->location = previous + (_scale[i] - previous) * (first->midvalue - sum) / d;
			}
			if (d)
				j = i;
			sum = new_sum;
		}

		for (; first != last; ++first)
			first->location = _scale[j];
	}

	template <typename S, typename Y>
	inline void histogram<S, Y>::reset()
	{	base_t::assign(size(), y_type());	}


	template <typename S, typename Y>
	inline histogram<S, Y> &operator +=(histogram<S, Y> &lhs, const histogram<S, Y> &rhs)
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

	template <typename S, typename Y>
	inline void interpolate(histogram<S, Y> &lhs, const histogram<S, Y> &rhs, float alpha)
	{
		if (lhs.get_scale() != rhs.get_scale())
			lhs.set_scale(rhs.get_scale());

		auto l = lhs.begin();
		auto r = rhs.begin();

		for (auto e = lhs.end(); l != e; ++l, ++r)
			*l += static_cast<Y>((*r - *l) * alpha);
	}
}
