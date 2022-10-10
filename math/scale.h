//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <cmath>
#include <stdexcept>

namespace math
{
	typedef unsigned int index_t;

#pragma pack(push, 1)
	template <typename T>
	class linear_scale
	{
	public:
		typedef T value_type;

	public:
		linear_scale();
		linear_scale(value_type near_value, value_type far_value, unsigned int samples_);

		value_type near_value() const;
		value_type far_value() const;
		index_t samples() const;
		bool operator ()(index_t &index, value_type value) const;
		value_type operator [](index_t index) const;

	private:
		// fast
		value_type _base;
		float _factor;
		unsigned int _samples;

		// slow
		float _step;
		value_type _near, _far;
	};

	template <typename T>
	class log_scale
	{
	public:
		typedef T value_type;

	public:
		log_scale();
		log_scale(value_type near_value, value_type far_value, unsigned int samples_);

		value_type near_value() const;
		value_type far_value() const;
		index_t samples() const;
		bool operator ()(index_t &index, value_type value) const;
		value_type operator [](index_t index) const;

	private:
		static float lg(value_type value);

	private:
		linear_scale<float> _inner;

		value_type _near, _far;
	};
#pragma pack(pop)


	template <typename T>
	inline linear_scale<T>::linear_scale()
		: _samples(0), _near(0), _far(0)
	{	}

	template <typename T>
	inline linear_scale<T>::linear_scale(value_type near_, value_type far_, unsigned int samples_)
		: _samples(samples_), _near(near_), _far(far_)
	{
		if (!samples_)
		{
			_near = value_type();
			_far = value_type();
		}
		else if (far_ <= near_)
		{
			throw std::invalid_argument("invalid closed-open interval is specified");
		}
		else if (_samples > 1)
		{
			_step = static_cast<float>(_far - _near) / (_samples - 1);
			_base = _near - static_cast<value_type>(0.5f * _step);
			_factor = 1.0f / _step;
		}
		else
		{
			_base = 0;
			_factor = 0.0f;
		}
	}

	template <typename T>
	inline T linear_scale<T>::near_value() const
	{	return _near;	}

	template <typename T>
	inline T linear_scale<T>::far_value() const
	{	return _far;	}

	template <typename T>
	inline index_t linear_scale<T>::samples() const
	{	return _samples;	}

	template <typename T>
	inline bool linear_scale<T>::operator ()(index_t &index, value_type value) const
	{
		if (auto samples_ = _samples)
		{
			const auto index_ = static_cast<int>((value - _base) * _factor);

			index = index_ < 0 ? 0u : index_ > static_cast<int>(--samples_) ? samples_ : static_cast<index_t>(index_);
			return true;
		}
		return false;
	}

	template <typename T>
	inline T linear_scale<T>::operator [](index_t index) const
	{	return _near + static_cast<value_type>(_step * index);	}


	template <typename T>
	inline log_scale<T>::log_scale()
		: _near(0), _far(0)
	{	}

	template <typename T>
	inline log_scale<T>::log_scale(value_type near_, value_type far_, unsigned int samples_)
		: _inner(lg(near_), lg(far_), samples_), _near(near_), _far(far_)
	{	}

	template <typename T>
	inline T log_scale<T>::near_value() const
	{	return _near;	}
	
	template <typename T>
	inline T log_scale<T>::far_value() const
	{	return _far;	}

	template <typename T>
	inline index_t log_scale<T>::samples() const
	{	return _inner.samples();	}

	template <typename T>
	inline bool log_scale<T>::operator ()(index_t &index, value_type value) const
	{	return value <= value_type() ? false : _inner(index, lg(value));	}

	template <typename T>
	inline T log_scale<T>::operator [](index_t index) const
	{	return static_cast<T>(std::pow(10.0f, _inner[index]));	}

	template <typename T>
	float log_scale<T>::lg(value_type value)
	{	return std::log10(static_cast<float>(value));	}



	template <typename ScaleT>
	inline bool scale_eq(const ScaleT &lhs, const ScaleT &rhs)
	{	return (lhs.samples() == rhs.samples()) & (lhs.near_value() == rhs.near_value()) & (lhs.far_value() == rhs.far_value());	}


	template <typename T>
	inline bool operator ==(const linear_scale<T> &lhs, const linear_scale<T> &rhs)
	{	return scale_eq(lhs, rhs);	}

	template <typename T>
	inline bool operator !=(const linear_scale<T> &lhs, const linear_scale<T> &rhs)
	{	return !(lhs == rhs);	}


	template <typename T>
	inline bool operator ==(const log_scale<T> &lhs, const log_scale<T> &rhs)
	{	return scale_eq(lhs, rhs);	}

	template <typename T>
	inline bool operator !=(const log_scale<T> &lhs, const log_scale<T> &rhs)
	{	return !(lhs == rhs);	}
}
