#include <views/any_key.h>

#include <common/hash.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace
{
	class controlled
	{
	public:
		controlled(size_t &copies)
			: _copies(copies)
		{	_copies++;	}

		controlled(const controlled &other)
			: _copies(other._copies)
		{	_copies++;	}

		~controlled()
		{	_copies--;	}

		bool operator ==(const controlled &/*rhs*/) const
		{	return true;	}

	private:
		void operator =(const controlled &rhs);

	private:
		size_t &_copies;
	};

#pragma pack(1)
	template <size_t n>
	struct filler
	{
		bool operator ==(const filler &/*rhs*/) const
		{	return false;	}

		char _dummy[n];
	};
}

namespace std
{
	template <>
	class hash<controlled>
	{
	public:
		size_t operator ()(const controlled &/*rhs*/) const
		{	return 0;	}
	};

	template <size_t n>
	class hash< filler<n> >
	{
	public:
		size_t operator ()(const filler<n> &/*rhs*/) const
		{	return 0;	}
	};
}

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			begin_test_suite( AnyKeyTests )
				test( ACopyIsStoredAndThenDestroyed )
				{
					// INIT
					size_t copies = 0;

					{
					// INIT / ACT
						any_key a1((controlled(copies)), hash<controlled>());

					// ASSERT
						assert_equal(1u, copies);

					// INIT / ACT
						any_key a2((controlled(copies)), hash<controlled>());

					// ASSERT
						assert_equal(2u, copies);

					// INIT / ACT
						any_key a3(a2);

					// ASSERT
						assert_equal(3u, copies);
					}

					// ASSERT
					assert_equal(0u, copies);
				}


				test( HashValueIsReturned )
				{
					// INIT / ACT / ASSERT
					assert_equal(hash<int>()(19213), any_key(19213, hash<int>()).hash());
					assert_equal(hash<int>()(1913), any_key(1913, hash<int>()).hash());
					assert_equal(hash<string>()("483jenf10d 0e2j"), any_key((string)"483jenf10d 0e2j", hash<string>()).hash());
					assert_equal(hash<string>()("17 02j"), any_key((string)"17 02j", hash<string>()).hash());
				}


				test( EqualityOperatorWorks )
				{
					// INIT / ACT / ASSERT
					assert_is_true(any_key(19213, hash<int>()) == any_key(19213, hash<int>()));
					assert_is_true(any_key(17, hash<int>()) == any_key(17, hash<int>()));
					assert_is_false(any_key(17, hash<int>()) == any_key(19213, hash<int>()));
					assert_is_false(any_key(19213, hash<int>()) == any_key(17, hash<int>()));

					assert_is_true(any_key((string)"lorem ipsum", hash<string>()) == any_key((string)"lorem ipsum", hash<string>()));
					assert_is_true(any_key((string)"amet dolor", hash<string>()) == any_key((string)"amet dolor", hash<string>()));
					assert_is_false(any_key((string)"amet dolor", hash<string>()) == any_key((string)"lorem ipsum", hash<string>()));
					assert_is_false(any_key((string)"lorem ipsum", hash<string>()) == any_key((string)"amet dolor", hash<string>()));
				}
			end_test_suite
		}
	}
}
