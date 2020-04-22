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

#include "platform.h"

#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

namespace micro_profiler
{
	namespace containers
	{
		using std::unordered_map;

		template < typename KeyT, typename ValueT, typename Hasher = std::hash<KeyT>, typename Comparer = std::equal_to<KeyT> >
		class unordered_map2 : std::list< std::pair<const KeyT, ValueT> >
		{
		private:
			enum { initial_nbuckets = 16, };
			typedef std::list< std::pair<const KeyT, ValueT> > base_t;

		public:
			typedef KeyT key_type;
			typedef ValueT mapped_type;
			typedef std::pair<const KeyT, ValueT> value_type;
			using typename base_t::const_iterator;
			using typename base_t::iterator;

		public:
			using base_t::begin;
			using base_t::end;
			using base_t::empty;
			using base_t::size;

		public:
			explicit unordered_map2(size_t nbuckets = initial_nbuckets)
			{	reset_buckets(nbuckets);	}

			unordered_map2(const unordered_map2 &other)
			{	*this = other;	}

			const unordered_map2 &operator =(const unordered_map2 &rhs)
			{
				base_t::clear();
				base_t::insert(end(), rhs.begin(), rhs.end());
				reset_buckets(rhs._buckets.size());
				for (iterator i = begin(); i != end(); ++i)
				{
					bucket_iterator b = get_bucket(i->first);

					if (!b->count++)
						b->start = i;
				}
				return *this;
			}

			mapped_type &operator [](const key_type &key)
			{
				const_bucket_iterator b = get_bucket(key);
				iterator i = b->start;

				for (unsigned int n = b->count; n; n--, i++)
					if (key == i->first)
						return i->second;
				return insert_empty(key);
			}

			void clear()
			{
				reset_buckets(_buckets.size());
				base_t::clear();
			}

			std::pair<iterator, bool> insert(const value_type &value)
			{
				iterator i = find(value.first);

				return i != end() ? std::make_pair(i, false) : std::make_pair(insert_always(value), true);
			}

#define UNORDERED_MAP_CV_DEF(cv, iterator_cv)\
			iterator_cv find(const KeyT &key) cv\
			{\
				const_bucket_iterator b = get_bucket(key);\
				iterator i = b->start;\
				for (unsigned int n = b->count; n; n--, i++)\
					if (key == i->first)\
						return i;\
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
			FORCE_NOINLINE mapped_type &insert_empty(const key_type &key)
			{	return insert_always(std::make_pair(key, mapped_type()))->second;	}

			iterator insert_always(const value_type &value)
			{
				bucket_iterator where = get_bucket(value.first);

				if (!where->count)
				{
					for (bucket_iterator next = where; ++next == _buckets_end ? where->start = end(), false : true; )
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
			{	return _buckets_begin + (_hasher(key) & _hash_mask);	}

			const_bucket_iterator get_bucket(const key_type &key) const
			{	return _buckets_begin + (_hasher(key) & _hash_mask);	}

			void reset_buckets(size_t nbuckets)
			{
				nbuckets = next_power_2(nbuckets);
				_buckets.assign(nbuckets, bucket());
				_buckets_begin = _buckets.begin(), _buckets_end = _buckets.end();
				_hash_mask = nbuckets - 1;
			}

			static size_t next_power_2(size_t value)
			{
				if (value > 1 && !((value - 1) & value))
					return value;
				for (size_t i = static_cast<size_t>(1) << (sizeof(size_t) * 8 - 2); i; i >>= 1)
					if (i < value)
						return i << 1;
				return 2;
			}

		private:
			bucket_iterator _buckets_begin, _buckets_end;
			size_t _hash_mask;
			buckets _buckets;
			Hasher _hasher;
		};
	}
}
