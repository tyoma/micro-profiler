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
#include "variant_scale.h"

#include <strmd/version.h>

namespace strmd
{
	template <typename T> struct version< math::linear_scale<T> > {	enum {	value = 1	};	};
	template <typename T> struct version< math::log_scale<T> > {	enum {	value = 1	};	};
	template <typename T> struct version< math::variant_scale<T> > {	enum {	value = 1	};	};
	template <typename S, typename Y> struct version< math::histogram<S, Y> > {	enum {	value = 1	};	};
}

namespace math
{
	namespace scontext
	{
		template <typename BufferT>
		struct additive
		{
			BufferT buffer;
		};

		template <typename BufferT>
		struct interpolating
		{
			float alpha;
			BufferT buffer;
		};
	}


	template <typename ArchiveT, typename ScaleT>
	inline void serialize_scale(ArchiveT &archive, ScaleT &data, unsigned int /*ver*/)
	{
		auto near_ = data.near_value();
		auto far_ = data.far_value();
		auto samples = data.samples();

		archive(near_);
		archive(far_);
		archive(samples);
		if ((near_ != data.near_value()) | (far_ != data.far_value()) | (samples != data.samples()))
			data = ScaleT(near_, far_, samples);
	}

	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, linear_scale<T> &data, unsigned int ver)
	{	serialize_scale(archive, data, ver);	}

	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, log_scale<T> &data, unsigned int ver)
	{	serialize_scale(archive, data, ver);	}

	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, variant_scale<T> &data, unsigned int /*ver*/)
	{	archive(static_cast<typename variant_scale<T>::base_t &>(data));	}

	template <typename ArchiveT, typename ScaleT, typename Y>
	inline void serialize(ArchiveT &archive, histogram<ScaleT, Y> &data, unsigned int /*ver*/)
	{
		archive(data._scale);
		archive(static_cast< std::vector<Y> &>(data));
	}

	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, T &data, scontext::additive<T> &context, unsigned int ver)
	{
		serialize(archive, context.buffer, ver);
		data += context.buffer;
	}

	template <typename ArchiveT, typename T>
	inline void serialize(ArchiveT &archive, T &data, scontext::interpolating<T> &context, unsigned int ver)
	{
		serialize(archive, context.buffer, ver);
		interpolate(data, context.buffer, context.alpha);
	}
}
