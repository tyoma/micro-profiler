#include <views/integrated_index.h>

#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace views
	{
		namespace tests
		{
			namespace
			{
				struct A
				{
					int a;
					double b;
					string c;
				};

				struct key_a
				{
					int operator ()(const A &value) const
					{	return value.a;	}
				};

				struct key_c
				{
					string operator ()(const A &value) const
					{	return value.c;	}
				};
			}

			begin_test_suite( IntegratedIndexTests )
				test( SameIndicesAreReturnedForTheSameKeyers )
				{
					typedef pair<int, string> type_1;

					// INIT
					const table<type_1> t1;
					const table<A> t2;
					table<A> t3;

					// ACT
					const immutable_unique_index<table<type_1>, key_first> &idx1 = unique_index<key_first>(t1);
					const immutable_index<table<type_1>, key_first> &idx2 = multi_index<key_first>(t1);
					const immutable_unique_index<table<A>, key_a> &idx3 = unique_index<key_a>(t2);
					const immutable_index<table<A>, key_a> &idx4 = multi_index<key_a>(t2);
					const immutable_unique_index<table<A>, key_c> &idx5 = unique_index<key_c>(t2);
					const immutable_index<table<A>, key_c> &idx6 = multi_index<key_c>(t2);
					immutable_unique_index<table<A>, key_a> &idx7 = unique_index<key_a>(t3);
					immutable_unique_index<table<A>, key_c> &idx8 = unique_index<key_c>(t3);

					// ACT / ASSERT
					assert_equal(&idx1, (&unique_index<key_first>(t1)));
					assert_equal(&idx2, (&multi_index<key_first>(t1)));
					assert_equal(&idx3, &unique_index<key_a>(t2));
					assert_equal(&idx4, &multi_index<key_a>(t2));
					assert_equal(&idx5, &unique_index<key_c>(t2));
					assert_equal(&idx6, &multi_index<key_c>(t2));
					assert_equal(&idx7, &unique_index<key_a>(t3));
					assert_equal(&idx8, &unique_index<key_c>(t3));
				}

			end_test_suite
		}
	}
}
