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

namespace math
{
	typedef unsigned int index_t;
	typedef long long int value_t;

	class scale
	{
	public:
		scale();
		scale(value_t near_value, value_t far_value, unsigned int samples_);

		value_t near_value() const;
		value_t far_value() const;
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



	inline scale::scale()
		: _samples(0), _near(0), _far(0)
	{	}

	inline scale::scale(value_t near, value_t far, unsigned int samples_)
		: _samples(samples_), _near(near), _far(far)
	{	reset();	}

	inline value_t scale::near_value() const
	{	return _near;	}

	inline value_t scale::far_value() const
	{	return _far;	}

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

	inline void scale::reset()
	{
		if (_samples > 1.00000012)
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


	inline bool scale::operator ==(const scale &rhs) const
	{	return !(*this != rhs);	}

	inline bool scale::operator !=(const scale &rhs) const
	{	return !!((_samples - rhs._samples) | (_near - rhs._near) | (_far - rhs._far));	}
}
