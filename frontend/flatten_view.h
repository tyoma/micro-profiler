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
	template <class U, class AccessT>
	class flatten_view
	{
	public:
		typedef typename AccessT::value_type value_type;
		typedef typename AccessT::value_type const_reference;

		class const_iterator
		{
		public:
			value_type operator *() const throw()
			{	return AccessT::get(*_l1, *_l2);	}

			const const_iterator &operator ++()
			{
				if (is_end() || ++_l2 == AccessT::end(*_l1))
				{
					++_l1;
					if (!is_end())
						_l2 = AccessT::begin(*_l1);
				}
				while (!is_end() && _l2 == AccessT::end(*_l1))
				{
					++_l1;
					if (!is_end())
						_l2 = AccessT::begin(*_l1);
				}
				return *this;
			}

			const_iterator operator ++(int)
			{
				auto prev = *this;

				++*this;
				return prev;
			}

			bool operator ==(const const_iterator &rhs) const
			{	return _l1 == rhs._l1 && (is_end() || _l2 == rhs._l2);	}

			bool operator !=(const const_iterator &rhs) const
			{	return !(*this == rhs);	}

		private:
			typedef typename U::const_iterator const_iterator_l1;
			typedef typename AccessT::nested_const_iterator const_iterator_l2;

		private:
			const_iterator(const_iterator_l1 l1, const_iterator_l1 l1_end)
				: _l1(l1), _l1_end(l1_end)
			{
				do
				{
					if (!is_end())
						_l2 = AccessT::begin(*_l1);
					else
						break;
				} while (_l2 == AccessT::end(*_l1) ? ++_l1, true : false);
			}

			const_iterator(const_iterator_l1 l1_end)
				: _l1(l1_end), _l1_end(l1_end)
			{	}

			bool is_end() const
			{	return _l1 == _l1_end;	}

		private:
			const_iterator_l1 _l1, _l1_end;
			const_iterator_l2 _l2;

		private:
			friend flatten_view;
		};

	public:
		flatten_view(const U &underlying)
			: _underlying(underlying)
		{	}

		const_iterator begin() const throw()
		{	return const_iterator(_underlying.begin(), _underlying.end());	}

		const_iterator end() const throw()
		{	return const_iterator(_underlying.end());	}

	private:
		const U &_underlying;
	};
}
