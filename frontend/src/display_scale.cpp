#include <frontend/display_scale.h>

#include <cmath>
#include <common/compiler.h>

using namespace std;

namespace micro_profiler
{
	display_scale::display_scale(const scale &scale_, value_t divisor, int pixel_size)
		: _near(static_cast<float>(scale_.near_value()) / divisor), _far(static_cast<float>(scale_.far_value()) / divisor)
	{
		_major = _far > _near ? major_tick(_far - _near) : 0.0f;
		_bin_width = static_cast<float>(pixel_size) / scale_.samples();
	}

	FORCE_NOINLINE float display_scale::next_tick(float value, float tick)
	{	return ceilf(value / tick) * tick;	}

	float display_scale::major_tick(float delta)
	{
		auto tick_size = powf(10.0f, floorf(log10f(delta)));
		return floorf(delta / tick_size) < 2 ? 0.1f * tick_size : tick_size;
	}

	display_scale::const_iterator display_scale::begin() const
	{
		if (_major)
		{
			tick t = {	_near, first	};
			return const_iterator(_major, _far, t);
		}
		return end();
	}

	display_scale::const_iterator display_scale::end() const
	{
		tick t = {	_far, _complete	};
		return const_iterator(_major, _far, t);
	}

	pair<float, float> display_scale::at(unsigned int index) const
	{
		float v = index * _bin_width;
		return make_pair(v, v + _bin_width);
	}


	display_scale::const_iterator::const_iterator(float major_, float far, tick tick_)
		: _major(major_), _far(far), _tick(tick_)
	{	}

	const display_scale::tick &display_scale::const_iterator::operator *() const
	{	return _tick;	}

	display_scale::const_iterator &display_scale::const_iterator::operator ++()
	{
		auto value = _tick.value;
		auto type = _tick.type;

		switch (type)
		{
		case first:	value = display_scale::next_tick(value, _major), type = major;	break;
		case major:	value += _major;	if (value >= _far) type = last, value = _far;	break;
		case last:	type = _complete; break;
		}
		_tick.value = value;
		_tick.type = type;
		return *this;
	}
}
