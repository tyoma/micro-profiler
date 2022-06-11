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
		}


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
