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

#include <vector>

namespace micro_profiler
{
	typedef unsigned int index_t;
	typedef long long value_t;

	class scale
	{
	public:
		scale(value_t near_value, value_t far_value, unsigned int segments);

		index_t segments() const;
		index_t operator ()(value_t value) const;

		bool operator ==(const scale &rhs) const;
		bool operator !=(const scale &rhs) const;

	private:
		void reset();

	private:
		value_t _near, _far;
		unsigned int _segments;
		float _scale, _base;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, scale &data, unsigned int ver);

		// TODO: implement logarithmic scaling
	};


	class histogram : std::vector<value_t>
	{
	public:
		histogram();

		void set_scale(const scale &scale_);
		const scale &get_scale() const;
		void add(value_t value);

		using std::vector<value_t>::size;
		using std::vector<value_t>::begin;
		using std::vector<value_t>::end;

		histogram &operator +=(const histogram &rhs);

	private:
		scale _scale;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, histogram &data, unsigned int ver);
	};



	inline scale::scale(value_t near, value_t far, unsigned int segments)
		: _near(near), _far(far), _segments(segments - 1)
	{	reset();	}

	inline index_t scale::segments() const
	{	return _segments + 1;	}

	inline index_t scale::operator ()(value_t value) const
	{
		return value >= _far ? _segments : value < _near ? 0u
			: static_cast<unsigned int>((static_cast<float>(value) - _base) * _scale);
	}

	inline bool scale::operator ==(const scale &rhs) const
	{	return !(*this != rhs);	}

	inline bool scale::operator !=(const scale &rhs) const
	{	return !!((_near - rhs._near) | (_far - rhs._far) | (_segments - rhs._segments));	}

	inline void scale::reset()
	{
		_scale = static_cast<float>(_far - _near) / _segments;
		_base = static_cast<float>(_near) - 0.5f * _scale;
		_scale = 1.0f / _scale;
	}


	inline histogram::histogram()
		: std::vector<value_t>(2), _scale(0, 1, 2)
	{	}

	inline void histogram::set_scale(const scale &scale_)
	{
		assign(scale_.segments(), value_t());
		_scale = scale_;
	}

	inline const scale &histogram::get_scale() const
	{	return _scale;	}

	inline void histogram::add(value_t value)
	{	(*this)[_scale(value)]++;	}

	inline histogram &histogram::operator +=(const histogram &rhs)
	{
		if (_scale != rhs.get_scale())
		{
			assign(rhs.begin(), rhs.end());
			_scale = rhs.get_scale();
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
