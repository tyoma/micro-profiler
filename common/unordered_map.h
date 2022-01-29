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

#include "compiler.h"

#include <functional>
#include <list>
#include <vector>
#include <unordered_map>

namespace micro_profiler
{
	namespace containers
	{
#if 1
		using std::unordered_map;
#else
		template < typename KeyT, typename ValueT, typename Hasher = std::hash<KeyT>, typename Comparer = std::equal_to<KeyT> >
		class unordered_map : std::list< std::pair<const KeyT, ValueT> >
		{
		private:
			enum { initial_nbuckets = 16, };
			typedef std::list< std::pair<const KeyT, ValueT> > base_t;

		public:
			typedef KeyT key_type;
			typedef ValueT mapped_type;
			typedef std::pair<const KeyT, ValueT> value_type;
			typedef unsigned short internal_size_type;
			using typename base_t::const_iterator;
			using typename base_t::iterator;

		public:
			using base_t::begin;
			using base_t::end;
			using base_t::empty;
			using base_t::size;

		public:
			explicit unordered_map(size_t nbuckets = initial_nbuckets)
				: _collision_allowance(2)
			{	reset_buckets(nbuckets);	}

			unordered_map(const unordered_map &other)
			{	*this = other;	}

			void collision_allowance(internal_size_type value)
			{	_collision_allowance = value;	}

			internal_size_type collision_allowance() const
			{	return _collision_allowance;	}

			size_t bucket_count() const
			{	return _buckets.size();	}

			void clear()
			{
				reset_buckets(_buckets.size());
				base_t::clear();
			}

			const unordered_map &operator =(const unordered_map &rhs)
			{
				_collision_allowance = rhs._collision_allowance;
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

#define UNORDERED_MAP_INDEX(key, exit1, exit2)\
				bucket_iterator b = get_bucket(key);\
				if (b->count && key == b->start->first)\
					return exit1;\
				internal_size_type n = b->count;\
				for (iterator i = b->start; n > 1; n--)\
				{\
					++i;\
					if (key == i->first)\
						return exit2;\
				}

			FORCE_INLINE mapped_type &operator [](const key_type &key)
			{
				UNORDERED_MAP_INDEX(key, b->start->second, i->second);
				return insert_empty(key);
			}

			std::pair<iterator, bool> insert(const value_type &value)
			{
				UNORDERED_MAP_INDEX(value.first, (std::pair<iterator, bool>(b->start, false)),
					(std::pair<iterator, bool>(i, false)));
				return std::make_pair(insert_always(value), true);
			}

#undef UNORDERED_MAP_INDEX


#define UNORDERED_MAP_CV_DEF(cv, iterator_cv)\
			iterator_cv find(const KeyT &key) cv\
			{\
				const_bucket_iterator b = get_bucket(key);\
				iterator i = b->start;\
				for (internal_size_type n = b->count; n; n--, i++)\
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
				internal_size_type count;
			};

			typedef std::vector<bucket> buckets;
			typedef typename std::vector<bucket>::iterator bucket_iterator;
			typedef typename std::vector<bucket>::const_iterator const_bucket_iterator;

		private:
			FORCE_NOINLINE mapped_type &insert_empty(const key_type &key)
			{	return insert_always(std::make_pair(key, mapped_type()))->second;	}

			iterator insert_always(const value_type &value)
			{
				const bucket_iterator where = get_bucket(value.first);

				if (where->count == _collision_allowance && has_non_unique_hash_values(*where))
					return rehash(), insert_always(value);
				iterator inserted = base_t::insert(find_insertion_point(where, _buckets_end), value);
				return where->count++, where->start = inserted;
			}

			void rehash()
			{
				reset_buckets(_buckets.size() << 1);

				iterator i = begin();
				size_t n = size();

				while (n--)
				{
					iterator from = i++;
					const bucket_iterator where = get_bucket(from->first);
					iterator insertion_point = find_insertion_point(where, _buckets_end);

					base_t::splice(insertion_point, *this, from);
					where->start = --insertion_point;
					where->count++;
				}
			}

			iterator find_insertion_point(const_bucket_iterator where, const_bucket_iterator buckets_end)
			{
				do
				{
					if (where->count)
						return where->start;
				} while (++where != buckets_end);
				return end();
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

			bool has_non_unique_hash_values(const bucket &b) const
			{
				internal_size_type n = b.count;
				const_iterator i = b.start;
				const size_t first_hash = _hasher(i->first);

				while (++i, --n)
				{
					if (_hasher(i->first) != first_hash)
						return true;
				}
				return false;
			}

		private:
			bucket_iterator _buckets_begin, _buckets_end;
			size_t _hash_mask;
			internal_size_type _collision_allowance;
			buckets _buckets;
			Hasher _hasher;
		};
#endif
	}
}
