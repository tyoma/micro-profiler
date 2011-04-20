#pragma once

#include <new>

namespace micro_profiler
{
	template <typename T>
	class pod_vector
	{
		union
		{
			T __unused1;
			int __unused2;
		};

		T *_buffer, *_next, *_capacity_limit;

		const pod_vector &operator =(const pod_vector &rhs);

	public:
		pod_vector(unsigned int initial_capacity = 10000);
		pod_vector(const pod_vector &other);
		~pod_vector() throw();

		void append(const T &element) throw();
		void clear() throw();

		const T *data() const throw();
		unsigned int size() const throw();
		unsigned int capacity() const throw();
	};


	template <typename T>
	inline pod_vector<T>::pod_vector(unsigned int initial_capacity)
		: _buffer(new T[initial_capacity]), _next(_buffer), _capacity_limit(_buffer + initial_capacity)
	{	}

	template <typename T>
	inline pod_vector<T>::pod_vector(const pod_vector &other)
		: _buffer(new T[other.capacity()]), _next(_buffer + other.size()), _capacity_limit(_buffer + other.capacity())
	{	memcpy(_buffer, other.data(), sizeof(T) * other.size());	}

	template <typename T>
	inline pod_vector<T>::~pod_vector() throw()
	{	delete []_buffer;	}

	template <typename T>
	__forceinline void pod_vector<T>::append(const T &element) throw()
	{
		if (_next == _capacity_limit)
		{
			unsigned int size = this->size();
			unsigned int new_capacity = capacity() + capacity() / 2;

			if (T *buffer = new(std::nothrow) T[new_capacity])
			{
				memcpy(buffer, _buffer, sizeof(T) * size);
				delete []_buffer;
				_buffer = buffer;
				_next = _buffer + size;
				_capacity_limit = _buffer + new_capacity;
			}
			else
				return;
		}
		*_next++ = element;
	}

	template <typename T>
	inline void pod_vector<T>::clear() throw()
	{	_next = _buffer;	}

	template <typename T>
	inline const T *pod_vector<T>::data() const throw()
	{	return _buffer;	}

	template <typename T>
	inline unsigned int pod_vector<T>::size() const throw()
	{	return _next - _buffer;	}

	template <typename T>
	inline unsigned int pod_vector<T>::capacity() const throw()
	{	return _capacity_limit - _buffer;	}
}
