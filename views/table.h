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

#include "table_events.h"

#include <common/compiler.h>
#include <unordered_map>
#include <vector>
#include <wpl/signal.h>

namespace micro_profiler
{
	namespace views
	{
		typedef const void *typeid_t;

		template <typename T>
		FORCE_NOINLINE typeid_t ctypeid() // Relies on function coalescing to avoid different id for the same type.
		{
			static T *dummy = nullptr;
			return &dummy;
		}

		template <typename T>
		struct default_constructor
		{
			T operator ()()
			{	return T();	}
		};

		template <typename F>
		struct component_type
		{
			typedef decltype((*static_cast<F *>(nullptr))()) type_;
			typedef typename std::remove_pointer<type_>::type type;
		};

		template < typename T, typename ConstructorT = default_constructor<T> >
		class table
		{
			typedef std::list<T> container_t;
			typedef typename container_t::iterator base_iterator_type;

		public:
			typedef typename container_t::const_iterator const_iterator;
			typedef const T &const_reference;
			typedef ConstructorT constructor_type;
			class iterator;
			typedef T &reference;
			class transacted_record;
			typedef T value_type;

		public:
			table(const ConstructorT &constructor = ConstructorT());

			void clear();

			std::size_t size() const;
			const_iterator begin() const;
			const_iterator end() const;
			iterator begin();
			iterator end();

			transacted_record create();

			template <typename CompConstructorT>
			const typename component_type<CompConstructorT>::type &component(const CompConstructorT &constructor) const;
			template <typename CompConstructorT>
			typename component_type<CompConstructorT>::type &component(const CompConstructorT &constructor);

		public:
			mutable wpl::signal<void (iterator irecord, bool new_)> changed;
			mutable wpl::signal<void (iterator irecord)> removed;
			mutable wpl::signal<void ()> cleared;
			mutable wpl::signal<void ()> invalidate;

		private:
			typedef unsigned int index_type;
			class iterator_base;

		private:
			template <typename CompConstructorT>
			table_events &construct_component(const CompConstructorT &constructor) const;

		private:
			ConstructorT _constructor;
			container_t _records;
			mutable std::unordered_map< typeid_t, std::unique_ptr<table_events> > _indices;

		private:
			template <typename ArchiveT, typename T2, typename ConstructorT2>
			friend void serialize(ArchiveT &archive, table<T2, ConstructorT2> &data);
			friend class iterator;
		};

		template <typename T, typename C>
		class table<T, C>::iterator : public base_iterator_type
		{
		public:
			transacted_record operator *() const;

		private:
			iterator(table &owner, base_iterator_type underlying)
				: base_iterator_type(underlying), _owner(&owner)
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
				: _table(other._table), _record(other._record), _new(other._new)
			{	}

			T &operator *()
			{	return *(base_iterator_type)_record;	}

			void commit()
			{	_table.changed(_record, _new);	}

			void remove()
			{
				_table.removed(_record);
				_table._records.erase(_record);
			}

		private:
			transacted_record(table &table_, iterator record, bool new_)
				: _table(table_), _record(record), _new(new_)
			{	}

			void operator =(const transacted_record &other);

		private:
			table &_table;
			iterator _record;
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
			invalidate();
		}

		template <typename T, typename C>
		inline std::size_t table<T, C>::size() const
		{	return _records.size();	}

		template <typename T, typename C>
		inline typename table<T, C>::const_iterator table<T, C>::begin() const
		{	return _records.begin();	}

		template <typename T, typename C>
		inline typename table<T, C>::const_iterator table<T, C>::end() const
		{	return _records.end();	}

		template <typename T, typename C>
		inline typename table<T, C>::iterator table<T, C>::begin()
		{	return iterator(*this, _records.begin());	}

		template <typename T, typename C>
		inline typename table<T, C>::iterator table<T, C>::end()
		{	return iterator(*this, _records.end());	}

		template <typename T, typename C>
		inline typename table<T, C>::transacted_record table<T, C>::create()
		{
			iterator record(*this, _records.insert(_records.end(), _constructor()));
			return transacted_record(*this, record, true);
		}

		template <typename T, typename C>
		template <typename CC>
		inline const typename component_type<CC>::type &table<T, C>::component(const CC &constructor) const
		{
			typedef typename component_type<CC>::type component_t;
			const auto i = _indices.find(ctypeid<component_t>());

			return static_cast<component_t &>(_indices.end() != i ? *i->second : construct_component(constructor));
		}

		template <typename T, typename C>
		template <typename CC>
		inline typename component_type<CC>::type &table<T, C>::component(const CC &constructor)
		{
			typedef typename component_type<CC>::type component_t;
			const auto i = _indices.find(ctypeid<component_t>());

			return static_cast<component_t &>(_indices.end() != i ? *i->second : construct_component(constructor));
		}

		template <typename T, typename C>
		template <typename CC>
		FORCE_NOINLINE inline table_events &table<T, C>::construct_component(const CC &constructor) const
		{
			typedef typename component_type<CC>::type component_t;
			const auto inserted = _indices.emplace(std::make_pair(ctypeid<component_t>(), nullptr));

			inserted.first->second.reset(constructor());
			return *inserted.first->second;
		}


		template <typename T, typename C>
		inline typename table<T, C>::transacted_record table<T, C>::iterator::operator *() const
		{	return transacted_record(*_owner, *this, false);	}
	}
}
