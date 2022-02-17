#include <frontend/dynamic_views.h>

#include "helpers.h"

#include <map>
#include <ut/assert.h>
#include <ut/test.h>
#include <views/index.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			class scope_table : vector<int>
			{
			public:
				using vector<int>::const_iterator;

			public:
				using vector<int>::begin;
				using vector<int>::end;

				void _clear()
				{	clear(), invalidate(123u);	}

				template <typename T>
				void _append(const T &from)
				{
					for (auto i = std::begin(from); i != std::end(from); ++i)
						push_back(*i);
					invalidate(1234u);
				}

			public:
				wpl::signal<void (size_t)> invalidate;
			};

			class entries_table : multimap<int, string>
			{
			public:
				using multimap<int, string>::const_iterator;
				using multimap<int, string>::const_reference;
				using multimap<int, string>::value_type;

			public:
				using multimap<int, string>::begin;
				using multimap<int, string>::end;
				using multimap<int, string>::equal_range;

				void _clear()
				{	clear(), invalidate();	}

				template <typename T>
				void _append(const T &from)
				{
					for (auto i = std::begin(from); i != std::end(from); ++i)
						insert(*i);
					invalidate();
				}

			public:
				wpl::signal<void ()> invalidate;
			};

			class trivial_index
			{
			public:
				typedef multimap<int, string>::const_iterator const_iterator;
				typedef multimap<int, string>::const_reference const_reference;
				typedef multimap<int, string>::value_type value_type;

			public:
				trivial_index(const entries_table &underlying)
					: _underlying(underlying)
				{	}

				pair<const_iterator, const_iterator> equal_range(int value) const
				{	return _underlying.equal_range(value);	}

				const_reference get(int, const_reference v2) const
				{	return v2;	}

			private:
				const entries_table &_underlying;
			};
		}

		begin_test_suite( ScopedViewTests )
			shared_ptr<scope_table> scope;
			shared_ptr<entries_table> entries;

			init( Init )
			{
				scope = make_shared<scope_table>();
				entries = make_shared<entries_table>();
			}


			test( EmptySetIsReturnedForEmptyScope )
			{
				// INIT / ACT
				auto scoped = make_scoped_view<trivial_index>(entries, scope);

				// ACT / ASSERT
				assert_equal(scoped->end(), scoped->begin());

				// INIT
				entries->_append(plural + make_pair(3, "Lorem") + make_pair(1, "amet") + make_pair(41, "dolor"));

				// ACT / ASSERT
				assert_equal(scoped->end(), scoped->begin());
			}


			test( ScopedItemsArePassedToOutput )
			{
				// INIT
				entries->_append(plural
					+ make_pair(3, "Lorem")
					+ make_pair(1, "ipsum")
					+ make_pair(41, "amet")
					+ make_pair(5, "dolor"));

				// INIT / ACT
				auto scoped = make_scoped_view<trivial_index>(entries, scope);

				scope->_append(plural + 3 + 41);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem")
					+ make_pair(41, (string)"amet"), *scoped);

				// INIT
				scope->_append(plural + 1);

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem")
					+ make_pair(1, (string)"ipsum")
					+ make_pair(41, (string)"amet"), *scoped);
			}


			test( InvalidationIsForwardedOnScopeInvalidation )
			{
				// INIT
				entries->_append(plural
					+ make_pair(3, "Lorem")
					+ make_pair(1, "ipsum")
					+ make_pair(41, "amet")
					+ make_pair(5, "dolor"));

				auto scoped = make_scoped_view<trivial_index>(entries, scope);
				auto invalidations = 0;
				auto conn = scoped->invalidate += [&] {	invalidations++;	};

				// ACT
				scope->_append(plural + 1);

				// ASSERT
				assert_equal(1, invalidations);

				// ACT
				scope->_append(plural + 41 + 5);

				// ASSERT
				assert_equal(2, invalidations);
			}


			test( InvalidationIsForwardedOnUnderlyingInvalidation )
			{
				// INIT
				scope->_append(plural + 1 + 3 + 5);

				auto scoped = make_scoped_view<trivial_index>(entries, scope);
				auto invalidations = 0;
				auto conn = scoped->invalidate += [&] {	invalidations++;	};

				// ACT
				entries->_append(plural
					+ make_pair(41, "amet")
					+ make_pair(5, "dolor"));

				// ASSERT
				assert_equal(1, invalidations);

				// ACT
				entries->_append(plural
					+ make_pair(43, "Lorem"));

				// ASSERT
				assert_equal(2, invalidations);
			}
		end_test_suite


		begin_test_suite( FilterViewTests )
			shared_ptr<entries_table> entries;

			init( Init )
			{	entries = make_shared<entries_table>();	}


			test( UnderlyingInvalidationInvokesInvalidation )
			{
				// INIT
				vector< pair<int, string> > collected;

				// INIT / ACT
				auto f = make_filter_view(entries);
				auto conn = f->invalidate += [&] {	collected.assign(f->begin(), f->end());	};

				// ACT
				entries->_append(plural
					+ make_pair(3, "Lorem")
					+ make_pair(1, "ipsum")
					+ make_pair(41, "amet")
					+ make_pair(5, "dolor"));

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem")
					+ make_pair(1, (string)"ipsum")
					+ make_pair(41, (string)"amet")
					+ make_pair(5, (string)"dolor"), collected);

				// INIT
				collected.clear();

				// ACT
				entries->_append(plural
					+ make_pair(7, "foo"));

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem")
					+ make_pair(1, (string)"ipsum")
					+ make_pair(41, (string)"amet")
					+ make_pair(5, (string)"dolor")
					+ make_pair(7, (string)"foo"), collected);
			}


			test( SettingFilterInvokesInvalidation )
			{
				// INIT
				vector< pair<int, string> > collected;

				entries->_append(plural
					+ make_pair(3, "Lorem")
					+ make_pair(1, "ipsum")
					+ make_pair(41, "amet")
					+ make_pair(5, "dolor"));

				// INIT / ACT
				auto f = make_filter_view(entries);
				auto conn = f->invalidate += [&] {	collected.assign(f->begin(), f->end());	};

				// ACT
				f->set_filter([] (pair<int, string> item) {
					return item.first == 3;
				});

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem"), collected);

				// ACT
				f->set_filter([] (pair<int, string> item) {	return item.first == 1 || item.first == 41;	});

				// ACT / ASSERT
				assert_equivalent(plural
					+ make_pair(1, (string)"ipsum")
					+ make_pair(41, (string)"amet"), collected);

				// ACT
				f->set_filter();

				// ASSERT
				assert_equivalent(plural
					+ make_pair(3, (string)"Lorem")
					+ make_pair(1, (string)"ipsum")
					+ make_pair(41, (string)"amet")
					+ make_pair(5, (string)"dolor"), collected);
			}

		end_test_suite
	}
}
