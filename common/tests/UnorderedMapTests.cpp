#include <common/unordered_map.h>

#include <string>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace containers
	{
		namespace tests
		{
			namespace
			{
				struct passthrough_hash
				{
					size_t operator ()(int v) const
					{	return static_cast<size_t>(v);	}
				};

				typedef unordered_map2<int, string> map1_t;
				typedef unordered_map2<string, int> map2_t;
				typedef unordered_map2<long long, int> map3_t;
			}

			begin_test_suite( UnorderedMapTests )
				test( NewContainerIsEmpty )
				{
					// INIT / ACT
					unordered_map2<int, int> c;

					// ACT / ASSERT
					assert_is_true(c.empty());
					assert_equal(0u, c.size());
				}


				test( AddedElementsAreAvailableByEnumeration )
				{
					// INIT
					map1_t c1;
					map2_t c2;

					// INIT / ACT / ASSERT
					assert_is_true(c1.insert(map1_t::value_type(1231, "lorem")).second);
					assert_is_true(c1.insert(map1_t::value_type(1, "ipsum")).second);
					assert_is_true(c1.insert(map1_t::value_type(19, "wtf?")).second);
					assert_is_true(c2.insert(map2_t::value_type("foo", 101002)).second);
					assert_is_true(c2.insert(map2_t::value_type("bar", 101000)).second);
					assert_is_true(c2.insert(map2_t::value_type("zoo", 104002)).second);
					assert_is_true(c2.insert(map2_t::value_type("b", 101092)).second);

					// ACT / ASSERT
					pair<int, string> reference1[] = {
						make_pair(1231, "lorem"), make_pair(1, "ipsum"), make_pair(19, "wtf?"),
					};
					pair<string, int> reference2[] = {
						make_pair("foo", 101002), make_pair("bar", 101000), make_pair("zoo", 104002), make_pair("b", 101092),
					};

					assert_is_false(c1.empty());
					assert_equal(3u, c1.size());
					assert_equivalent(reference1, c1);
					assert_is_false(c2.empty());
					assert_equal(4u, c2.size());
					assert_equivalent(reference2, c2);
				}


				test( FoundElementIsReturned )
				{
					// INIT
					map1_t c1;
					map2_t c2;

					c1.insert(map1_t::value_type(1231, "lorem"));
					c1.insert(map1_t::value_type(1, "ipsum"));
					c1.insert(map1_t::value_type(19, "wtf?"));
					c2.insert(map2_t::value_type("foo", 101002));
					c2.insert(map2_t::value_type("bar", 101000));
					c2.insert(map2_t::value_type("zoo", 104002));
					c2.insert(map2_t::value_type("b", 101092));

					// ACT / ASSERT
					assert_equal(map1_t::value_type(1231, "lorem"), *c1.find(1231));
					assert_equal(map1_t::value_type(1, "ipsum"), *c1.find(1));
					assert_equal(map1_t::value_type(19, "wtf?"), *c1.find(19));
					assert_equal(map2_t::value_type("foo", 101002), *c2.find("foo"));
					assert_equal(map2_t::value_type("bar", 101000), *c2.find("bar"));
					assert_equal(map2_t::value_type("zoo", 104002), *c2.find("zoo"));
					assert_equal(map2_t::value_type("b", 101092), *c2.find("b"));

					// ACT
					c1.find(1231)->second = "LOREM";
					c2.find("zoo")->second = 123;

					// ASSERT
					assert_equal(map1_t::value_type(1231, "LOREM"), *c1.find(1231));
					assert_equal(map2_t::value_type("zoo", 123), *c2.find("zoo"));
				}


				test( FoundElementIsReturnedForConstantMap )
				{
					// INIT
					map1_t c1_;
					const map1_t &c1 = c1_;
					map2_t c2_;
					const map2_t &c2 = c2_;

					c1_.insert(map1_t::value_type(1231, "lorem"));
					c1_.insert(map1_t::value_type(1, "ipsum"));
					c1_.insert(map1_t::value_type(19, "wtf?"));
					c2_.insert(map2_t::value_type("foo", 101002));
					c2_.insert(map2_t::value_type("bar", 101000));
					c2_.insert(map2_t::value_type("zoo", 104002));
					c2_.insert(map2_t::value_type("b", 101092));

					// ACT / ASSERT
					assert_equal(map1_t::value_type(1231, "lorem"), *c1.find(1231));
					assert_equal(map1_t::value_type(1, "ipsum"), *c1.find(1));
					assert_equal(map1_t::value_type(19, "wtf?"), *c1.find(19));
					assert_equal(map2_t::value_type("foo", 101002), *c2.find("foo"));
					assert_equal(map2_t::value_type("bar", 101000), *c2.find("bar"));
					assert_equal(map2_t::value_type("zoo", 104002), *c2.find("zoo"));
					assert_equal(map2_t::value_type("b", 101092), *c2.find("b"));
				}


				test( InsertionReturnsTheSameIteratorReturnedByFind )
				{
					// INIT
					map1_t c;

					// ACT / ASSERT
					map1_t::iterator i1 = c.insert(make_pair(171912, "aaabbb")).first;
					map1_t::const_iterator i2 = c.insert(make_pair(12233445, "aaabbb")).first;

					// ACT / ASSERT
					assert_equal(c.find(171912), i1);
					assert_equal(c.find(12233445), i2);
				}


				test( InsertionOfExistingValuesReturnsIteratorToThemAndDoesNotChangeValue )
				{
					// INIT
					map1_t c;

					map1_t::iterator i1 = c.insert(make_pair(171912, "aaabbb")).first;
					map1_t::const_iterator i2 = c.insert(make_pair(12233445, "zzzqqq")).first;

					// ACT / ASSERT
					pair<map1_t::iterator, bool> ii1 = c.insert(make_pair(171912, "zbzbz"));
					pair<map1_t::iterator, bool> ii2 = c.insert(make_pair(12233445, "z"));

					// ACT / ASSERT
					assert_equal(i1, ii1.first);
					assert_is_false(ii1.second);
					assert_equal("aaabbb", i1->second);
					assert_equal(i2, ii2.first);
					assert_is_false(ii2.second);
					assert_equal("zzzqqq", i2->second);
				}


				test( RecordsAreGrouppedByHashValueThenByOrderOfInsertion )
				{
					// This test implies that initial buckets amount is 16 and filling quantity is at least 3

					// INIT
					unordered_map2<int, string, passthrough_hash> c1, c2;

					// ACT
					c1.insert(make_pair(0, "a"));
					c1.insert(make_pair(1, "b"));
					c1.insert(make_pair(5, "c"));
					c1.insert(make_pair(6, "d"));
					c1.insert(make_pair(17, "e")); // second
					c1.insert(make_pair(37, "f")); // second
					c1.insert(make_pair(21, "f")); // third

					c2.insert(make_pair(16, "q"));
					c2.insert(make_pair(24, "m"));
					c2.insert(make_pair(40, "r")); // second
					c2.insert(make_pair(64, "p")); // second
					c2.insert(make_pair(65, "z"));

					// ASSERT
					pair<const int, string> reference1[] = {
						make_pair(0, "a"),
						make_pair(17, "e"), make_pair(1, "b"),
						make_pair(21, "f"), make_pair(37, "f"), make_pair(5, "c"),
						make_pair(6, "d"),
					};
					pair<const int, string> reference2[] = {
						make_pair(64, "p"), make_pair(16, "q"),
						make_pair(65, "z"),
						make_pair(40, "r"), make_pair(24, "m"),
					};

					assert_equal(reference1, c1);
					assert_equal(reference2, c2);
				}


				test( AllEntriesFromTheSameBucketAreScanned )
				{
					// INIT
					unordered_map2<int, string, passthrough_hash> c;

					c.insert(make_pair(0, "a"));
					c.insert(make_pair(1, "b"));
					c.insert(make_pair(5, "c"));
					c.insert(make_pair(6, "d"));
					c.insert(make_pair(17, "e")); // second
					c.insert(make_pair(37, "f")); // second
					c.insert(make_pair(21, "q")); // third

					// ACT / ASSERT
					assert_not_equal(c.end(), c.find(1));
					assert_equal(c.end(), c.find(33));
					assert_equal("b", c.find(1)->second);
					assert_not_equal(c.end(), c.find(5));
					assert_equal("c", c.find(5)->second);
					assert_not_equal(c.end(), c.find(21));
					assert_equal("q", c.find(21)->second);
					assert_not_equal(c.end(), c.find(37));
					assert_equal("f", c.find(37)->second);
					assert_equal(c.end(), c.find(53));
				}
			end_test_suite
		}
	}
}
