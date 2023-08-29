#pragma once

#include <utility>

namespace micro_profiler
{
	template <typename T>
	struct generate_value
	{
		static T value();
	};

	template <typename F, typename T>
	struct invoke_result_1
	{
		typedef decltype(generate_value<F>::value()(generate_value<T>::value())) type;
	};

	template <typename T>
	class nullable
	{
	public:
		nullable();
		nullable(const nullable &other);
		explicit nullable(const T &from);

		~nullable();

		template <typename F>
		nullable<typename invoke_result_1<F, T>::type> and_then(F &&transform) const;

		bool has_value() const;
		T &operator *();
		const T &operator *() const;

		nullable &operator =(const nullable &from);
		nullable &operator =(const T &from);

	private:
		unsigned char _buffer[sizeof(T)];
		bool _has_value;
	};

	template <typename T>
	class nullable<T &>
	{
	public:
		nullable();
		nullable(const nullable &other);
		explicit nullable(T &from);

		bool has_value() const;
		T &operator *();
		const T &operator *() const;

	private:
		nullable &operator =(const nullable &from);

	private:
		T *_object;
	};

	struct null_
	{
		template <typename T>
		operator const nullable<T> &() const;
	} const null;



	template <typename T>
	inline nullable<T>::nullable()
		: _has_value(false)
	{	}

	template <typename T>
	inline nullable<T>::nullable(const nullable &other)
		: _has_value(other._has_value)
	{
		if (_has_value)
			new (_buffer) T(*other);
	}

	template <typename T>
	inline nullable<T>::nullable(const T &from)
		: _has_value(true)
	{	new(_buffer) T(from);	}

	template <typename T>
	inline nullable<T>::~nullable()
	{
		if (_has_value)
			(**this).~T();
	}

	template <typename T>
	template <typename F>
	inline nullable<typename invoke_result_1<F, T>::type> nullable<T>::and_then(F &&transform) const
	{
		typedef nullable<typename invoke_result_1<F, T>::type> result_type;

		return _has_value ? result_type(transform(**this)) : result_type();
	}

	template <typename T>
	inline bool nullable<T>::has_value() const
	{	return _has_value;	}

	template <typename T>
	inline T &nullable<T>::operator *()
	{	return *reinterpret_cast<T *>(_buffer);	}

	template <typename T>
	inline const T &nullable<T>::operator *() const
	{	return *reinterpret_cast<const T *>(_buffer);	}

	template <typename T>
	inline nullable<T> &nullable<T>::operator =(const nullable &from)
	{
		if (_has_value)
			(**this).~T(), _has_value = false;
		if (from.has_value())
			new(_buffer) T(*from), _has_value = true;
		return *this;
	}

	template <typename T>
	inline nullable<T> &nullable<T>::operator =(const T &from)
	{
		if (_has_value)
			(**this).~T(); // TODO: reset value presence to protect from construction exception below.
		new(_buffer) T(from);
		_has_value = true;
		return *this;
	}


	template <typename T>
	inline nullable<T &>::nullable()
		: _object(nullptr)
	{	}

	template <typename T>
	inline nullable<T &>::nullable(const nullable &other)
		: _object(other._object)
	{	}

	template <typename T>
	inline nullable<T &>::nullable(T &from)
		: _object(&from)
	{	}

	template <typename T>
	inline bool nullable<T &>::has_value() const
	{	return !!_object;	}

	template <typename T>
	inline T &nullable<T &>::operator *()
	{	return *_object;	}

	template <typename T>
	inline const T &nullable<T &>::operator *() const
	{	return *_object;	}


	template <typename T>
	inline bool operator ==(const nullable<T> &lhs, const nullable<T> &rhs)
	{	return !lhs.has_value() && !rhs.has_value() ? true : lhs.has_value() != rhs.has_value() ? false : *lhs == *rhs;	}


	template <typename T>
	inline null_::operator const nullable<T> &() const
	{
		static nullable<T> v;
		return v;
	}


	template <typename T>
	inline bool operator ==(const null_ &/*lhs*/, const nullable<T> &rhs)
	{	return !rhs.has_value();	}

	template <typename T>
	inline bool operator ==(const nullable<T> &lhs, const null_ &/*rhs*/)
	{	return !lhs.has_value();	}
}
