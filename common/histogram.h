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
	typedef long long int value_t;

	class scale
	{
	public:
		scale();
		scale(value_t near_value, value_t far_value, unsigned int samples_);

		index_t samples() const;
		bool operator ()(index_t &index, value_t value) const;

		bool operator ==(const scale &rhs) const;
		bool operator !=(const scale &rhs) const;

	private:
		void reset();

	private:
		value_t _base;
		float _scale;
		unsigned int _samples;

		value_t _near, _far;

	private:
		template <typename ArchiveT>
		friend void serialize(ArchiveT &archive, scale &data, unsigned int ver);

		// TODO: implement logarithmic scaling
	};


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



	histogram &operator +=(histogram &lhs, const histogram &rhs);
	void interpolate(histogram &lhs, const histogram &rhs, float alpha);


	inline scale::scale()
		: _samples(0), _near(0), _far(0)
	{	}

	inline index_t scale::samples() const
	{	return _samples;	}

	inline bool scale::operator ()(index_t &index, value_t value) const
	{
		if (auto samples_ = _samples)
		{
			const auto index_ = static_cast<int>((value - _base) * _scale);

			index = index_ < 0 ? 0u : index_ > static_cast<int>(--samples_) ? samples_ : static_cast<index_t>(index_);
			return true;
		}
		return false;
	}

	inline bool scale::operator ==(const scale &rhs) const
	{	return !(*this != rhs);	}

	inline bool scale::operator !=(const scale &rhs) const
	{	return !!((_samples - rhs._samples) | (_near - rhs._near) | (_far - rhs._far));	}


	inline const scale &histogram::get_scale() const
	{	return _scale;	}

	inline void histogram::add(value_t at, value_t d)
	{
		index_t index = 0;

		if (_scale(index, at))
			(*this)[index] += d;
	}
}
