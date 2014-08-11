#pragma once

#include <algorithm>

namespace assert
{
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
}
