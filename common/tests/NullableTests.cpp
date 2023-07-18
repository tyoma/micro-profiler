#include <common/nullable.h>

#include <memory>
#include <string>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class instance
			{
			public:
				instance(int &references)
					: _references(&references), _moved_away(false)
				{	references = 101;	}

				instance(const instance &other)
					: _references(other._references), _moved_away(false)
				{	*_references += 101;	}

				instance(instance &&other) throw()
					: _references(other._references), _moved_away(false)
				{	other._moved_away = true, *_references += 1;	}

				~instance()
				{
					*_references -= 1;
					if (!_moved_away)
						*_references -= 100;
					_references = nullptr;
				}

			private:
				instance &operator =(const instance &rhs);

			private:
				int *_references; // decimal CCNN - CC is the number of cloned states, NN is the amount of cloned objects.
				bool _moved_away;
			};
		}

		begin_test_suite( NullableTests )
			test( DefaultConstructedNullableHasNoValue )
			{
				// INIT / ACT
				nullable<int> ni;
				nullable<string> ns;
				nullable<int &> nri;

				// ACT / ASSERT
				assert_is_false(ni.has_value());
				assert_is_false(ns.has_value());
				assert_is_false(nri.has_value());

				// INIT / ACT
				nullable<int> ni2(ni);
				nullable<string> ns2(ns);
				nullable<int &> nri2(nri);

				// ACT / ASSERT
				assert_is_false(ni2.has_value());
				assert_is_false(ns2.has_value());
				assert_is_false(nri2.has_value());
			}


			test( CopiedNullablesHasValue )
			{
				// INIT / ACT
				nullable<int> ni(13);
				nullable<string> ns(string("lorem ipsum"));
				nullable<const int &> nri(1718);

				// ACT / ASSERT
				assert_is_true(ni.has_value());
				assert_is_true(ns.has_value());
				assert_is_true(nri.has_value());

				// INIT / ACT
				nullable<int> ni2(ni);
				nullable<string> ns2(ns);
				nullable<const int &> nri2(nri);

				// ACT / ASSERT
				assert_is_true(ni2.has_value());
				assert_is_true(ns2.has_value());
				assert_is_true(nri2.has_value());
			}


			test( MovingANullableMovesItsValue )
			{
				// INIT
				nullable<string> n;
				nullable<string> nv(string("lorem ipsum"));
				nullable<const int &> nri(130);

				// INIT / ACT
				nullable<string> mn(move(n));
				nullable<string> mnv(move(nv));
				nullable<const int &> mnri(move(nri));

				// ACT / ASSERT
				assert_is_false(n.has_value());
				assert_is_false(mn.has_value());
				assert_is_false(nv.has_value());
				assert_is_true(mnv.has_value());
				assert_is_false(nri.has_value());
				assert_is_true(mnri.has_value());
			}


			test( ContainedObjectLifetimeIsGuardedByTheNullable )
			{
				// INIT
				auto references = 0;

				// INIT / ACT
				unique_ptr< nullable<instance> > n(new nullable<instance>(instance(references)));

				// ASSERT
				assert_equal(101, references);

				// INIT / ACT
				unique_ptr< nullable<instance> > cn(new nullable<instance>(*n));

				// ASSERT
				assert_equal(202, references);

				// ACT
				n.reset();

				// ASSERT
				assert_equal(101, references);

				// ACT
				cn.reset();

				// ASSERT
				assert_equal(0, references);
			}


			test( MovingObjectToNullableAtConstructionWorks )
			{
				// INIT
				auto references = 0;
				unique_ptr< nullable<instance> > n;

				{
					instance original(references);

				// ACT
					n.reset(new nullable<instance>(move(original)));

				// ASSERT
					assert_equal(102, references);

				// ACT
				}

				// ASSERT
				assert_equal(101, references);
			}


			test( MovingNullableToNullableAtConstructionWorks )
			{
				// INIT
				auto references = 0;
				unique_ptr< nullable<instance> > n;

				{
					nullable<instance> original((instance(references)));

					// ACT
					n.reset(new nullable<instance>(move(original)));

					// ASSERT
					assert_equal(101, references);
					assert_is_false(original.has_value());

					// ACT
				}

				// ASSERT
				assert_equal(101, references);
			}


			test( DereferencingOfANonNullValueReturnsStableReference )
			{
				// INIT
				string somevalue = "lorem ipsum amet dolor";
				nullable<int> nint(1928);
				nullable<string> nstring(string("Sic transit gloria mundi"));
				const nullable<string &> nrstring(somevalue);

				// INIT / ACT
				auto &rint = *nint;
				auto &rstring = *nstring;
				auto &rrstring = *nrstring;
				auto &rint2 = *nint;
				auto &rstring2 = *nstring;
				auto &rrstring2 = *nrstring;
				auto &crrstring = *nrstring;

				// ASSERT
				assert_equal(1928, rint);
				assert_equal(&rint, &rint2);
				assert_equal("Sic transit gloria mundi", rstring);
				assert_equal(&rstring, &rstring2);
				assert_equal(&somevalue, &rrstring);
				assert_equal(&rrstring, &rrstring2);
				assert_equal(&crrstring, &somevalue);
				assert_is_true(is_const<remove_reference<decltype(crrstring)>::type>::value);
			}


			test( FromEmptyCopyAssigningUnderlyingWorks )
			{
				// INIT
				auto references = 0;
				auto ivalue = 1928;
				string svalue = "lorem ipsum amet dolor";
				instance insvalue(references);
				nullable<int> nint;
				nullable<string> nstring;
				nullable<instance> ninstance;

				// ACT / ASSERT
				assert_equal(&nint, &(nint = ivalue));
				nstring = svalue;
				ninstance = insvalue;

				// ASSERT
				assert_is_true(nint.has_value());
				assert_equal(1928, *nint);
				assert_is_true(nstring.has_value());
				assert_equal("lorem ipsum amet dolor", *nstring);
				assert_is_true(ninstance.has_value());
				assert_equal(202, references);
			}


			test( FromEmptyMoveAssigningUnderlyingWorks )
			{
				// INIT
				auto references = 0;
				instance insvalue(references);
				nullable<instance> ninstance;

				// ACT / ASSERT
				ninstance = move(insvalue);

				// ASSERT
				assert_equal(102, references);
			}


			test( PreviousValueIsDestroyedOnCopyAssignment )
			{
				// INIT
				auto references1 = 0;
				auto references2 = 0;
				nullable<instance> ninstance((instance(references1)));
				instance insvalue2(references2);

				// ACT / ASSERT
				ninstance = insvalue2;

				// ASSERT
				assert_equal(0, references1);
				assert_equal(202, references2);
			}


			test( AndThenWorksAsExpected )
			{
				// INIT
				nullable<int> ni;
				auto called = 0;
				auto cvt = [&] (const int &value) -> string {
					char buffer[100];
					sprintf(buffer, "%d", value);
					called++;
					return buffer;
				};

				// INIT / ACT
				nullable<string> ns = ni.and_then(cvt);

				// ASSERT
				assert_is_false(ns.has_value());
				assert_equal(0, called);

				// INIT
				ni = 192811;

				// INIT / ACT
				nullable<string> ns2 = ni.and_then(cvt);

				// ASSERT
				assert_is_true(ns2.has_value());
				assert_equal("192811", *ns2);
			}


			struct X
			{
				int a;
				int b;

				bool operator ==(const X &/*rhs*/) const
				{	return false;	}
			};

			test( AndThenWorksAsExpectedForReferences )
			{
				// INIT
				X some = {	1213, 19001	};
				nullable<X> empty_x;
				nullable<X> valued_x(move(some));
				auto cvt = [] (const X &value) -> const int & {	return value.b;	};

				// ACT / ASSERT
				assert_is_false(empty_x.and_then(cvt).has_value());
				assert_is_true(valued_x.and_then(cvt).has_value());
				assert_equal(&(*valued_x).b, &*valued_x.and_then(cvt));
			}


			test( EmptyNullableIsNotEqualToAValued )
			{
				// INIT
				const nullable<bool> empty;
				const nullable<X &> rempty;
				X sample;

				// ACT / ASSERT
				assert_is_false(null == nullable<bool>(false));
				assert_is_false(nullable<bool>(false) == null);
				assert_is_false(null == nullable<bool>(true));
				assert_is_false(nullable<bool>(true) == null);
				assert_is_false(null == nullable<X &>(sample));
				assert_is_false(nullable<X &>(sample) == null);

				assert_is_false(empty == nullable<bool>(false));
				assert_is_false(nullable<bool>(false) == empty);
				assert_is_false(empty == nullable<bool>(true));
				assert_is_false(nullable<bool>(true) == empty);
				assert_is_false(rempty == nullable<X &>(sample));
				assert_is_false(nullable<X &>(sample) == rempty);
			}


			test( EmptyNullableEqualsToOtherEmptyOptional )
			{
				// INIT
				nullable<bool> nn(null);
				nullable<bool> empty;
				nullable<bool> cempty;
				nullable<X &> rempty;
				nullable<X &> rcempty;

				// ACT / ASSERT
				assert_is_true(null == nn);
				assert_is_true(nn == null);
				assert_is_true(null == nullable<bool>());
				assert_is_true(nullable<bool>() == null);
				assert_is_true(nullable<bool>() == nullable<bool>());
				assert_is_true(empty == nullable<bool>());
				assert_is_true(nullable<bool>() == empty);
				assert_is_true(cempty == nullable<bool>());
				assert_is_true(nullable<bool>() == cempty);

				assert_is_true(null == nullable<X &>());
				assert_is_true(nullable<X &>() == null);
				assert_is_true(nullable<X &>() == nullable<X &>());
				assert_is_true(rempty == nullable<X &>());
				assert_is_true(nullable<X &>() == rempty);
				assert_is_true(rcempty == nullable<X &>());
				assert_is_true(nullable<X &>() == rcempty);
			}


			test( ValuedNullableEqualsToAnotherWithTheSameUnderlying )
			{
				// INIT
				auto v1 = 17;
				auto v2 = 191;
				auto v3 = 17;

				// ACT / ASSERT
				assert_is_true(nullable<bool>(false) == nullable<bool>(false));
				assert_is_false(nullable<bool>(true) == nullable<bool>(false));
				assert_is_true(nullable<int>(19182371) == nullable<int>(19182371));
				assert_is_false(nullable<int>(19182371) == nullable<int>(3141));
				assert_is_false(nullable<int &>(v1) == nullable<int &>(v2));
				assert_is_true(nullable<int &>(v1) == nullable<int &>(v3));
				assert_is_true(nullable<int &>(v1) == nullable<int &>(v1));
			}
		end_test_suite
	}
}
