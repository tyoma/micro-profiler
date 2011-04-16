#pragma once

#include <memory.h>

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

		unsigned int _size, _capacity;
		T *_buffer;

		const pod_vector &operator =(const pod_vector &rhs);

	public:
		pod_vector(unsigned int initial_capacity = 10000);
		pod_vector(const pod_vector &other);
		~pod_vector();

		void append(const T &element);
		void clear();

		const T *data() const;
		unsigned int size() const;
	};


	template <typename T>
	inline pod_vector<T>::pod_vector(unsigned int initial_capacity)
		: _size(0), _capacity(initial_capacity), _buffer(new T[initial_capacity])
	{	}

	template <typename T>
	inline pod_vector<T>::pod_vector(const pod_vector &other)
		: _size(other._size), _capacity(other._capacity), _buffer(new T[other._capacity])
	{	memcpy(_buffer, other._buffer, sizeof(T) * other._size);	}

	template <typename T>
	inline pod_vector<T>::~pod_vector()
	{	delete []_buffer;	}

	template <typename T>
	__forceinline void pod_vector<T>::append(const T &element)
	{
		if (_size == _capacity)
		{
			unsigned int capacity = _capacity + _capacity / 2;
			T *buffer = new T[capacity];

			memcpy(buffer, _buffer, sizeof(T) * _size);
			_capacity = capacity;
		}
		_buffer[_size++] = element;
	}

	template <typename T>
	inline void pod_vector<T>::clear()
	{	_size = 0;	}

	template <typename T>
	inline const T *pod_vector<T>::data() const
	{	return _buffer;	}

	template <typename T>
	inline unsigned int pod_vector<T>::size() const
	{	return _size;	}
}
