#pragma once

#if defined(MP_MT_GENERIC)
	#include <chrono>

	namespace mt
	{
		using std::chrono::duration;
		using std::chrono::milliseconds;
	}

#else
	namespace mt
	{
		template <typename RepT, typename RatioT = void>
		class duration
		{
		public:
			duration();
			explicit duration(RepT value);

			RepT count() const;

		private:
			RepT _value;
		};

		typedef duration<long long, void> milliseconds;



		template <typename RepT, typename RatioT>
		inline duration<RepT, RatioT>::duration()
		{	}

		template <typename RepT, typename RatioT>
		inline duration<RepT, RatioT>::duration(RepT value)
			: _value(value)
		{	}

		template <typename RepT, typename RatioT>
		inline RepT duration<RepT, RatioT>::count() const
		{	return _value;	}


		template <typename RepT, typename RatioT>
		inline bool operator <(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() < rhs.count();	}

		template <typename RepT, typename RatioT>
		inline bool operator <=(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() <= rhs.count();	}

		template <typename RepT, typename RatioT>
		inline bool operator >(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() > rhs.count();	}

		template <typename RepT, typename RatioT>
		inline bool operator >=(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() >= rhs.count();	}

		template <typename RepT, typename RatioT>
		inline bool operator ==(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() == rhs.count();	}

		template <typename RepT, typename RatioT>
		inline bool operator !=(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return lhs.count() != rhs.count();	}

		template <typename RepT, typename RatioT>
		inline duration<RepT, RatioT> operator +(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return duration<RepT, RatioT>(lhs.count() + rhs.count());	}

		template <typename RepT, typename RatioT>
		inline duration<RepT, RatioT> operator -(duration<RepT, RatioT> lhs, duration<RepT, RatioT> rhs)
		{	return duration<RepT, RatioT>(lhs.count() - rhs.count());	}

		template <typename T, typename RepT, typename RatioT>
		inline duration<RepT, RatioT> operator *(T lhs, duration<RepT, RatioT> rhs)
		{	return duration<RepT, RatioT>(static_cast<RepT>(lhs * rhs.count()));	}

		template <typename T, typename RepT, typename RatioT>
		inline duration<RepT, RatioT> operator *(duration<RepT, RatioT> lhs, T rhs)
		{	return rhs * lhs;	}
	}

#endif

namespace strmd
{
	template <typename StreamT, typename PackerT> class serializer;
	template <typename StreamT, typename PackerT> class deserializer;

	template <typename StreamT, typename PackerT, typename RepT, typename RatioT>
	inline void serialize(serializer<StreamT, PackerT> &archive, mt::duration<RepT, RatioT> data)
	{	archive(data.count());	}

	template <typename StreamT, typename PackerT, typename RepT, typename RatioT>
	inline void serialize(deserializer<StreamT, PackerT> &archive, mt::duration<RepT, RatioT> &data)
	{
		RepT v;

		archive(v);
		data = mt::duration<RepT, RatioT>(v);
	}
}
