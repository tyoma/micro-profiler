//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <list>
#include <unordered_map>
#include <vector>

namespace micro_profiler
{
	namespace containers
	{
		using std::unordered_map;

		template < typename KeyT, typename ValueT, typename Hasher = std::hash<KeyT> >
		class unordered_map2 : std::list< std::pair<const KeyT, ValueT> >
		{
		private:
			enum { initial_buckets = 16, };
			typedef std::list< std::pair<const KeyT, ValueT> > base_t;

		public:
			typedef KeyT key_type;
			typedef std::pair<const KeyT, ValueT> value_type;
			using typename base_t::const_iterator;
			using typename base_t::iterator;

		public:
			using base_t::begin;
			using base_t::end;
			using base_t::empty;
			using base_t::size;

		public:
			unordered_map2()
				: _buckets(initial_buckets)
			{	}

			std::pair<iterator, bool> insert(const value_type &value)
			{
				iterator i = find(value.first);

				return i != end() ? std::make_pair(i, false) : std::make_pair(insert_internal(value), true);
			}

#define UNORDERED_MAP_CV_DEF(cv, iterator_cv)\
			iterator_cv find(const KeyT &key) cv\
			{\
				const bucket &b = *get_bucket(key);\
				if (unsigned int n = b.count)\
				{\
					if (key == b.start->first)\
						return b.start;\
					for (iterator_cv i = b.start; --n; )\
						if (key == (++i)->first)\
							return i;\
				}\
				return end();\
			}

			UNORDERED_MAP_CV_DEF(const, const_iterator);
			UNORDERED_MAP_CV_DEF(, iterator);

#undef UNORDERED_MAP_CV_DEF

		private:
			struct bucket
			{
				bucket()
					: count(0u)
				{	}

				iterator start;
				unsigned int count;
			};

			typedef std::vector<bucket> buckets;
			typedef typename std::vector<bucket>::iterator bucket_iterator;
			typedef typename std::vector<bucket>::const_iterator const_bucket_iterator;

		private:
			iterator insert_internal(const value_type &value)
			{
				bucket_iterator where = get_bucket(value.first);

				if (!where->count)
				{
					for (bucket_iterator next = where; ++next == _buckets.end() ? where->start = end(), false : true; )
					{
						if (next->count)
						{
							where->start = next->start;
							break;
						}
					}
				}
				//else if (where->count == max_bucket_occupation)
				//{
				//	rehash();
				//	return insert_internal(value);
				//}
				where->start = base_t::insert(where->start, value);
				where->count++;
				return where->start;
			}

			bucket_iterator get_bucket(const key_type &key)
			{	return _buckets.begin() + (_hasher(key) & (_buckets.size() - 1));	}

			const_bucket_iterator get_bucket(const key_type &key) const
			{	return _buckets.begin() + (_hasher(key) & (_buckets.size() - 1));	}

		private:
			buckets _buckets;
			Hasher _hasher;
		};
	}
}
