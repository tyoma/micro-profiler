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

#include "histogram.h"

#include <strmd/version.h>

namespace strmd
{
	template <> struct version<math::scale> {	enum {	value = 1	};	};
	template <> struct version<math::histogram> {	enum {	value = 1	};	};
}

namespace math
{
	namespace scontext
	{
		struct additive
		{
			histogram histogram_buffer;
		};

		struct interpolating
		{
			float alpha;
			histogram histogram_buffer;
		};
	}


	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, scale &data, unsigned int /*ver*/)
	{
		auto tmp = data;

		archive(tmp._near);
		archive(tmp._far);
		archive(tmp._samples);
		if (tmp != data)
			tmp.reset(), data = tmp;
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, histogram &data, unsigned int /*ver*/)
	{
		archive(data._scale);
		archive(static_cast<std::vector<value_t> &>(data));
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, histogram &data, scontext::additive &context, unsigned int ver)
	{
		serialize(archive, context.histogram_buffer, ver);
		data += context.histogram_buffer;
	}

	template <typename ArchiveT>
	inline void serialize(ArchiveT &archive, histogram &data, scontext::interpolating &context, unsigned int ver)
	{
		serialize(archive, context.histogram_buffer, ver);
		interpolate(data, context.histogram_buffer, context.alpha);
	}
}
