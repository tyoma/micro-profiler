#include <frontend/nested_transform.h>

#include <list>
#include <map>
#include <unordered_map>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			struct statistics1_t
			{
				typedef map<int, string> callees_type;
				typedef vector<double> callers_type;

				callees_type callees;
				callers_type callers;
			};

			struct statistics2_t
			{
				typedef list<int> callees_type;
				typedef unordered_map<int, int> callers_type;

				list<int> callees;
				unordered_map<int, int> callers;
			};
		}

		begin_test_suite( CalleesStatisticsTransformTests )
			typedef map<int, statistics1_t> map11_t;
			typedef unordered_map<string, statistics1_t> map12_t;
			typedef map<int, statistics2_t> map21_t;
			typedef map<string, statistics2_t> map22_t;

			test( NoLinkedStatisticsIsProvidedForEmptyCallees )
			{
				// INIT
				map11_t m11;
				map12_t m12;
				map21_t m21;
				map22_t m22;

				m11[314];
				m12["abc"];
				m21[273];
				m22["lorem"];

				// INIT / ACT
				callees_transform<map11_t> t11(m11);
				callees_transform<map12_t> t12(m12);
				callees_transform<map21_t> t21(m21);
				callees_transform<map22_t> t22(m22);

				// ACT / ASSERT
				assert_equal(t11.end(314), t11.begin(314));
				assert_equal(t12.end("abc"), t12.begin("abc"));
				assert_equal(t21.end(273), t21.begin(273));
				assert_equal(t22.end("lorem"), t22.begin("lorem"));
			}


			test( NoLinkedStatisticsIsProvidedForMissingEntries )
			{
				// INIT
				map11_t m;
				callees_transform<map11_t> t(m);

				// ACT / ASSERT
				assert_equal(t.end(314), t.begin(314));
			}


			test( NestedContainerIteratorsAreProvidedAsRange )
			{
				// INIT
				map11_t m;
				callees_transform<map11_t> t(m);

				m[150].callees[1] = "a";
				m[150].callees[2] = "b";
				m[151].callees[3] = "A";
				m[151].callees[7] = "B";
				m[151].callees[8] = "B";

				// ACT / ASSERT
				assert_equal(m[150].callees.begin(), t.begin(150));
				assert_equal(m[150].callees.end(), t.end(150));
				assert_equal(m[151].callees.begin(), t.begin(151));
				assert_equal(m[151].callees.end(), t.end(151));
			}
		end_test_suite


		begin_test_suite( CallersStatisticsTransformTests )
			typedef map<int, statistics1_t> map11_t;
			typedef unordered_map<string, statistics1_t> map12_t;
			typedef map<int, statistics2_t> map21_t;
			typedef map<string, statistics2_t> map22_t;

			test( NoLinkedStatisticsIsProvidedForEmptyCallers )
			{
				// INIT
				map11_t m11;
				map12_t m12;
				map21_t m21;
				map22_t m22;

				m11[314];
				m12["abc"];
				m21[273];
				m22["lorem"];

				// INIT / ACT
				callers_transform<map11_t> t11(m11);
				callers_transform<map12_t> t12(m12);
				callers_transform<map21_t> t21(m21);
				callers_transform<map22_t> t22(m22);

				// ACT / ASSERT
				assert_equal(t11.end(314), t11.begin(314));
				assert_equal(t12.end("abc"), t12.begin("abc"));
				assert_equal(t21.end(273), t21.begin(273));
				assert_equal(t22.end("lorem"), t22.begin("lorem"));
			}


			test( NoLinkedStatisticsIsProvidedForMissingEntries )
			{
				// INIT
				map11_t m;
				callers_transform<map11_t> t(m);

				// ACT / ASSERT
				assert_equal(t.end(314), t.begin(314));
			}


			test( NestedContainerIteratorsAreProvidedAsRange )
			{
				// INIT
				map11_t m;
				callers_transform<map11_t> t(m);

				m[150].callers.push_back(0.1);
				m[150].callers.push_back(1.1);
				m[150].callers.push_back(3.1);
				m[151].callers.push_back(7.1);
				m[151].callers.push_back(9.1);

				// ACT / ASSERT
				assert_equal(m[150].callers.begin(), t.begin(150));
				assert_equal(m[150].callers.end(), t.end(150));
				assert_equal(m[151].callers.begin(), t.begin(151));
				assert_equal(m[151].callers.end(), t.end(151));
			}
		end_test_suite

	}
}
