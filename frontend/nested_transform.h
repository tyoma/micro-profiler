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

namespace micro_profiler
{
	template <typename U>
	class callees_transform
	{
	public:
		typedef typename U::key_type key_type;
		typedef decltype(static_cast<typename U::mapped_type *>(0)->callees) nested_container_type;
		typedef typename nested_container_type::const_iterator nested_const_iterator;
		typedef typename nested_container_type::const_reference const_reference;
		typedef typename nested_container_type::value_type value_type;

	public:
		callees_transform(const U &underlying)
			: _underlying(underlying)
		{	}

		nested_const_iterator begin(const key_type &key) const
		{
			auto i = _underlying.find(key);
			return i != _underlying.end() ? i->second.callees.begin() : _empty_nested.end();
		}

		nested_const_iterator end(const key_type &key) const
		{
			auto i = _underlying.find(key);
			return i != _underlying.end() ? i->second.callees.end() : _empty_nested.end();
		}

		template <typename T1, typename T2>
		static const T2 &get(const T1 &, const T2 &value)
		{	return value;	}

	private:
		const U &_underlying;
		const nested_container_type _empty_nested;
	};


	template <typename U>
	class callers_transform
	{
	public:
		typedef typename U::key_type key_type;
		typedef decltype(static_cast<typename U::mapped_type *>(0)->callers) nested_container_type;
		typedef typename nested_container_type::const_iterator nested_const_iterator;
		typedef typename nested_container_type::const_reference const_reference;
		typedef typename nested_container_type::value_type value_type;

	public:
		callers_transform(const U &underlying)
			: _underlying(underlying)
		{	}

		nested_const_iterator begin(const key_type &key) const
		{
			auto i = _underlying.find(key);
			return i != _underlying.end() ? i->second.callers.begin() : _empty_nested.end();
		}

		nested_const_iterator end(const key_type &key) const
		{
			auto i = _underlying.find(key);
			return i != _underlying.end() ? i->second.callers.end() : _empty_nested.end();
		}

		template <typename T1, typename T2>
		static const T2 &get(const T1 &, const T2 &value)
		{	return value;	}

	private:
		const U &_underlying;
		const nested_container_type _empty_nested;
	};
}
