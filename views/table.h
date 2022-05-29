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

#include "table_component.h"

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
			class const_iterator;
			typedef const T &const_reference;
			typedef ConstructorT constructor_type;
			typedef T &reference;
			class transacted_record;
			typedef T value_type;

		public:
			table(const ConstructorT &constructor = ConstructorT());

			void clear();

			std::size_t size() const;
			const_iterator begin() const;
			const_iterator end() const;

			transacted_record create();
			transacted_record modify(const_iterator record);

			template <typename CompConstructorT>
			const typename component_type<CompConstructorT>::type &component(const CompConstructorT &constructor) const;
			template <typename CompConstructorT>
			typename component_type<CompConstructorT>::type &component(const CompConstructorT &constructor);

		public:
			mutable wpl::signal<void (const_iterator record, bool new_)> changed;
			mutable wpl::signal<void (const_iterator record)> removed;
			mutable wpl::signal<void ()> cleared;
			mutable wpl::signal<void ()> invalidate;

		private:
			template <typename CompConstructorT>
			table_component &construct_component(const CompConstructorT &constructor) const;

		private:
			ConstructorT _constructor;
			mutable container_t _records;
			mutable std::unordered_map< typeid_t, std::unique_ptr<table_component> > _components;

		private:
			template <typename ArchiveT, typename T2, typename ConstructorT2>
			friend void serialize(ArchiveT &archive, table<T2, ConstructorT2> &data);
		};

		template <typename T, typename C>
		class table<T, C>::const_iterator
		{
		public:
			typedef typename table<T, C>::const_reference const_reference;
			typedef typename base_iterator_type::difference_type difference_type;
			typedef typename base_iterator_type::iterator_category iterator_category;
			typedef const typename table<T, C>::value_type *pointer;
			typedef const_reference reference;
			typedef typename table<T, C>::value_type value_type;

		public:
			const_iterator()
			{	}

			const_reference operator *() const
			{	return *_underlying;	}

			bool operator !=(const const_iterator &rhs) const
			{	return _underlying != rhs._underlying;	}

			bool operator ==(const const_iterator &rhs) const
			{	return _underlying == rhs._underlying;	}

			void operator ++()
			{	++_underlying;	}

		private:
			const_iterator(base_iterator_type underlying)
				: _underlying(underlying)
			{	}

		private:
			base_iterator_type _underlying;

		private:
			friend class table;
		};

		template <typename T, typename C>
		class table<T, C>::transacted_record
		{
		public:
			transacted_record(transacted_record &&other)
				: _table(other._table), _record(other._record), _new(other._new)
			{	}

			T &operator *()
			{	return *_record;	}

			void commit()
			{	_table.changed(_record, _new);	}

			void remove()
			{
				_table.removed(_record);
				_table._records.erase(_record);
			}

		private:
			transacted_record(table &table_, base_iterator_type record, bool new_)
				: _table(table_), _record(record), _new(new_)
			{	}

			void operator =(const transacted_record &other);

		private:
			table &_table;
			base_iterator_type _record;
			bool _new;

		private:
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
		inline typename table<T, C>::transacted_record table<T, C>::create()
		{	return transacted_record(*this, _records.insert(_records.end(), _constructor()), true);	}

		template <typename T, typename C>
		inline typename table<T, C>::transacted_record table<T, C>::modify(const_iterator record)
		{	return transacted_record(*this, record._underlying, false);	}

		template <typename T, typename C>
		template <typename CC>
		inline const typename component_type<CC>::type &table<T, C>::component(const CC &constructor) const
		{
			typedef typename component_type<CC>::type component_t;
			const auto i = _components.find(ctypeid<component_t>());

			return static_cast<component_t &>(_components.end() != i ? *i->second : construct_component(constructor));
		}

		template <typename T, typename C>
		template <typename CC>
		inline typename component_type<CC>::type &table<T, C>::component(const CC &constructor)
		{
			typedef typename component_type<CC>::type component_t;
			const auto i = _components.find(ctypeid<component_t>());

			return static_cast<component_t &>(_components.end() != i ? *i->second : construct_component(constructor));
		}

		template <typename T, typename C>
		template <typename CC>
		FORCE_NOINLINE inline table_component &table<T, C>::construct_component(const CC &constructor) const
		{
			typedef typename component_type<CC>::type component_t;
			const auto inserted = _components.emplace(std::make_pair(ctypeid<component_t>(), nullptr));

			inserted.first->second.reset(constructor());
			return *inserted.first->second;
		}
	}
}
