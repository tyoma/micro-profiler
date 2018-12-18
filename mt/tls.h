#pragma once

namespace mt
{
	class tls_base
	{
	public:
		tls_base();
		~tls_base();

		void *get() const;
		void set(void *value);

	private:
		tls_base(const tls_base &other);
		const tls_base &operator =(const tls_base &rhs);

	private:
		unsigned int _index;
	};

	template <typename T>
	struct tls : private tls_base
	{
		T *get() const;
		void set(T *value);
	};


	// tls<T> - inline definitions
	template <typename T>
	inline T *tls<T>::get() const
	{	return static_cast<T *>(tls_base::get());	}

	template <typename T>
	inline void tls<T>::set(T *value)
	{	tls_base::set(value);	}
}
