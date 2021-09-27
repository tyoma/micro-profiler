#pragma once

#include <common/histogram.h>

namespace micro_profiler
{
	class display_scale
	{
	public:
		enum tick_type {	first, last, major, minor, _complete /*not used externally*/,	};
		struct tick;
		typedef tick value_type;
		class const_iterator;

	public:
		display_scale(const scale &scale_, value_t divisor, int pixel_size);

		static float next_tick(float value, float tick);
		static float major_tick(float delta);

		const_iterator begin() const;
		const_iterator end() const;
		std::pair<float, float> at(unsigned int index) const;

	private:
		float _near, _far, _major, _bin_width;
	};

	struct display_scale::tick
	{
		float value;
		tick_type type;
	};

	class display_scale::const_iterator
	{
	public:
		const_iterator(float major_, float far, tick tick_);

		const tick &operator *() const;
		const_iterator &operator ++();

	private:
		float _major, _far;
		tick _tick;
	};



	inline bool operator ==(display_scale::tick lhs, display_scale::tick rhs)
	{	return lhs.value == rhs.value && lhs.type == rhs.type;	}

	inline bool operator ==(const display_scale::const_iterator &lhs, const display_scale::const_iterator &rhs)
	{	return *lhs == *rhs;	}

	inline bool operator !=(const display_scale::const_iterator &lhs, const display_scale::const_iterator &rhs)
	{	return !(lhs == rhs);	}
}

namespace std
{
	template<>
	struct iterator_traits<micro_profiler::display_scale::const_iterator>
	{
		typedef forward_iterator_tag iterator_category;
		typedef micro_profiler::display_scale::tick value_type;
		typedef int difference_type;
		typedef value_type reference;
	};
}
