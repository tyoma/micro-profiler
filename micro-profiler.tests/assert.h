#pragma once

#include <algorithm>
#include <vector>

namespace assert
{
	template <typename T>
	struct type_traits
	{
		typedef typename T::value_type value_type;
	};

	template <typename T, size_t N>
	struct type_traits<T[N]>
	{
		typedef T value_type;
	};

	template <typename T>
	struct type_traits<T*>
	{
		typedef T value_type;
	};

	template <typename T>
	inline typename T::const_iterator cbegin(const T &c)
	{	return c.begin();	}

	template <typename T>
	inline typename T::const_iterator cend(const T &c)
	{	return c.end();	}

	template <typename T, size_t N>
	inline const T *cbegin(T (&a)[N])
	{	return a;	}

	template <typename T, size_t N>
	inline const T *cend(T (&a)[N])
	{	return a + N;	}


   template <typename T1, typename T2>
   inline void sequences_equal(const T1 &reference, const T2 &actual)
	{
		using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
      using namespace std;

      if (lexicographical_compare(cbegin(reference), cend(reference), cbegin(actual), cend(actual))
         != lexicographical_compare(cbegin(actual), cend(actual), cbegin(reference), cend(reference)))
         Assert::Fail();
	}

   template <typename T1, typename T2>
   inline void sequences_equivalent(const T1 &reference, const T2 &actual)
	{
		using namespace Microsoft::VisualStudio::TestTools::UnitTesting;
      using namespace std;

		vector<typename type_traits<T1>::value_type> vreference(cbegin(reference), cend(reference));
		vector<typename type_traits<T1>::value_type> vactual(cbegin(actual), cend(actual));

		sort(vreference.begin(), vreference.end());
		sort(vactual.begin(), vactual.end());

      if (!(vreference == vactual))
         Assert::Fail();
	}
}
