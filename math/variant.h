//	Copyright (c) 2011-2023 by Artem A. Gevorkyan (gevorkyan.org)
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

namespace math
{
	template <typename T, typename BaseT = void>
	class variant;

	template <typename T>
	class variant<T, void>
	{
	public:
		variant(const T &from)
			: _type(0), _value(from)
		{	}

		unsigned int get_type() const
		{	return _type;	}

		void change_type(unsigned int type)
		{	_type = type;	}

		template <typename V>
		typename V::return_type visit(const V &visitor) const
		{	return visitor(_value);	}

		template <typename V>
		typename V::return_type visit(const V &visitor)
		{	return visitor(_value);	}

	protected:
		enum {	id = 0	};

	protected:
		variant()
		{	}

	protected:
		int _type;

	private:
		T _value;
	};

	template <typename T, typename NextT, typename BaseT>
	class variant< T, variant<NextT, BaseT> > : public variant<NextT, BaseT>
	{
	private:
		typedef variant<NextT, BaseT> base_t;

	public:
		template <typename OtherT>
		variant(const OtherT &from)
			: base_t(from)
		{	}

		variant(const T &from)
			: _value(from)
		{	_type = id;	}

		template <typename V>
		typename V::return_type visit(const V &visitor) const
		{	return _type == id ? visitor(_value) : base_t::visit(visitor);	}

		template <typename V>
		typename V::return_type visit(const V &visitor)
		{	return _type == id ? visitor(_value) : base_t::visit(visitor);	}

	protected:
		enum {	id = base_t::id + 1	};

	protected:
		variant()
		{	}

	protected:
		using base_t::_type;

	private:
		T _value;
	};


	template <typename R = void>
	struct static_visitor
	{
		typedef R return_type;
	};


	template <typename ArchiveT>
	struct serializing_visitor
	{
		typedef void return_type;

		template <typename T>
		void operator ()(T &value) const
		{	archive(value);	}

		ArchiveT &archive;
	};

	template <typename V>
	struct equality_visitor : static_visitor<bool>
	{
		equality_visitor(const V &rhs_)
			: rhs(rhs_)
		{	}

		template <typename T>
		bool operator ()(const T &lhs) const
		{	return rhs.visit(inner<T>(lhs));	}

		template <typename T>
		struct inner : static_visitor<bool>
		{
			inner(const T &lhs_)
				: lhs(lhs_)
			{	}

			bool operator ()(const T &rhs) const
			{	return lhs == rhs;	}

			template <typename OtherT>
			bool operator ()(const OtherT &) const {	return false;	}

			const T &lhs;
		};

		const V &rhs;
	};



	template <typename T, typename BaseT>
	inline bool operator ==(const variant<T, BaseT> &lhs, const variant<T, BaseT> &rhs)
	{	return lhs.visit(equality_visitor< variant<T, BaseT> >(rhs));	}

	template <typename T, typename BaseT>
	inline bool operator !=(const variant<T, BaseT> &lhs, const variant<T, BaseT> &rhs)
	{	return !(lhs == rhs);	}

	template <typename ArchiveT, typename T, typename BaseT>
	inline void serialize(ArchiveT &archive, variant<T, BaseT> &data)
	{
		serializing_visitor<ArchiveT> v = {	archive	};
		auto type_id = data.get_type();

		archive(type_id);
		data.change_type(type_id);
		data.visit(v);
	}
}
