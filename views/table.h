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

#include <vector>
#include <wpl/signal.h>

namespace micro_profiler
{
	namespace views
	{
		template <typename T>
		struct default_constructor
		{
			T operator ()() const
			{	return T();	}
		};

		template < typename T, typename ConstructorT = default_constructor<T> >
		class table
		{
		public:
			class const_iterator;
			typedef const T &const_reference;
			class iterator;
			typedef T &reference;
			class transacted_record;
			typedef T value_type;

		public:
			table(const ConstructorT &constructor = ConstructorT());

			void clear();

			unsigned size() const;
			const_iterator begin() const;
			const_iterator end() const;
			iterator begin();
			iterator end();

			transacted_record create();

		public:
			mutable wpl::signal<void (iterator irecord, bool new_)> changed;
			mutable wpl::signal<void ()> cleared;

		private:
			typedef std::vector<T> container_type;
			typedef unsigned int index_type;
			class iterator_base;

		private:
			const ConstructorT _constructor;
			container_type _records;
		};

		template <typename T, typename C>
		class table<T, C>::iterator_base
		{
		public:
			typedef std::forward_iterator_tag iterator_category;
			typedef T value_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::ptrdiff_t distance_type;

		public:
			void operator ++()
			{	++_index;	}

			bool operator ==(const iterator_base &rhs) const
			{	return _index == rhs._index;	}

			bool operator !=(const iterator_base &rhs) const
			{	return _index != rhs._index;	}

		protected:
			iterator_base(index_type index)
				: _index(index)
			{	}

			index_type get_index() const
			{	return _index;	}

		private:
			index_type _index;
		};

		template <typename T, typename C>
		class table<T, C>::const_iterator : public iterator_base
		{
		public:
			typedef const T *pointer;
			typedef const T &reference;

		public:
			reference operator *() const
			{	return (*_container)[this->get_index()];	}

		private:
			const_iterator(const container_type &container_, index_type index)
				: iterator_base(index), _container(&container_)
			{	}

		private:
			const container_type *_container;

		private:
			friend class table;
		};

		template <typename T, typename C>
		class table<T, C>::iterator : public iterator_base
		{
		public:
			transacted_record operator *() const;

			operator const_iterator() const
			{	return const_iterator(_owner->_records, this->get_index());	}

		private:
			iterator(table &owner, index_type index)
				: iterator_base(index), _owner(&owner)
			{	}

		private:
			table *_owner;

		private:
			friend class table;
			friend class transacted_record;
		};

		template <typename T, typename C>
		class table<T, C>::transacted_record
		{
		public:
			transacted_record(transacted_record &&other)
				: _table(other._table), _index(other._index), _new(other._new)
			{	}

			T &operator *()
			{	return _table._records[_index];	}

			void commit()
			{	_table.changed(iterator(_table, _index), _new);	}

		private:
			transacted_record(table &table_, index_type index, bool new_)
				: _table(table_), _index(index), _new(new_)
			{	}

			void operator =(const transacted_record &other);

		private:
			table &_table;
			const index_type _index;
			bool _new;

		private:
			friend class iterator;
			friend class table;
		};



		template <typename T, typename C>
		inline table<T, C>::table(const C &constructor)
			: _constructor(constructor)
		{	}

		template <typename T, typename C>
		inline void table<T, C>::clear()
		{
			_records.clear();
			cleared();
		}

		template <typename T, typename C>
		inline unsigned table<T, C>::size() const
		{	return _records.size();	}

		template <typename T, typename C>
		inline typename table<T, C>::const_iterator table<T, C>::begin() const
		{	return const_iterator(_records, 0);	}

		template <typename T, typename C>
		inline typename table<T, C>::const_iterator table<T, C>::end() const
		{	return const_iterator(_records, static_cast<index_type>(_records.size()));	}

		template <typename T, typename C>
		inline typename table<T, C>::iterator table<T, C>::begin()
		{	return iterator(*this, 0);	}

		template <typename T, typename C>
		inline typename table<T, C>::iterator table<T, C>::end()
		{	return iterator(*this, static_cast<index_type>(_records.size()));	}

		template <typename T, typename C>
		inline typename table<T, C>::transacted_record table<T, C>::create()
		{
			auto index = static_cast<index_type>(_records.size());

			_records.push_back(_constructor());
			return transacted_record(*this, index, true);
		}


		template <typename T, typename C>
		inline typename table<T, C>::transacted_record table<T, C>::iterator::operator *() const
		{	return transacted_record(*_owner, this->get_index(), false);	}
	}
}
