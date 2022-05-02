#include <frontend/table_model_impl.h>

#include "helpers.h"

#include <common/formatting.h>
#include <frontend/key.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace
	{
		struct organization
		{
			unsigned org_id;
			unsigned parent_id;
			string name;
			unsigned workforce;
		};
	}

	template <>
	struct key_traits<organization>
	{
		typedef unsigned key_type;

		static key_type get_key(const organization &value)
		{	return value.org_id;	}
	};

	namespace tests
	{
		namespace
		{
			struct organization_context
			{
				function<const organization *(unsigned org_id)> by_id;
			};

			struct organization_hierarchy
			{
				organization_hierarchy(const organization_context &context)
					: _by_id(context.by_id)
				{	}

				static bool same_parent(const organization &lhs, const organization &rhs)
				{	return lhs.parent_id == rhs.parent_id;	}

				vector<unsigned> path(const organization &o) const
				{
					vector<unsigned> path;

					for (auto item = &o; item; item = _by_id(item->parent_id))
						path.push_back(item->org_id);
					reverse(path.begin(), path.end());
					return path;
				}

				const organization &lookup(unsigned id) const
				{	return *_by_id(id);	}

				static bool total_less(const organization &lhs, const organization &rhs)
				{	return lhs.org_id < rhs.org_id;	}

			private:
				const function<const organization *(unsigned org_id)> _by_id;
			};

			template <typename GetTextT, typename CompareT>
			column_definition<organization, organization_context> make_column(const GetTextT &get_text,
				const CompareT &compare)
			{
				column_definition<organization, organization_context> c = {
					string(), agge::style_modifier::empty, 0, agge::align_near, get_text, compare,
				};

				return c;
			}

			vector< column_definition<organization, organization_context> > create_columns()
			{
				return plural
					+ make_column([] (agge::richtext_t &t, const organization_context &, size_t, const organization &o) {	t << o.name.c_str();	},
						[] (const organization_context &, const organization &lhs, const organization &rhs) {	return strcmp(lhs.name.c_str(), rhs.name.c_str());	})
					+ make_column([] (agge::richtext_t &t, const organization_context &, size_t, const organization &o) {	itoa<10>(t, o.workforce);	},
						[] (const organization_context &, const organization &lhs, const organization &rhs) {	return (int)lhs.workforce - (int)rhs.workforce;	});
			}

			template <typename T, size_t n>
			shared_ptr< vector<typename remove_const<T>::type> > make_vector(T (&a)[n])
			{	return make_shared< vector<typename remove_const<T>::type> >(a, a + n);	}

			template <typename U, typename CtxT>
			shared_ptr< table_model_impl<wpl::richtext_table_model, U, CtxT> > make_table(shared_ptr<U> underlying,
				const CtxT &context)
			{	return make_shared< table_model_impl<wpl::richtext_table_model, U, CtxT> >(underlying, context);	}

			organization_hierarchy access_hierarchy(const organization_context &context, const organization *)
			{	return organization_hierarchy(context);	}
		}

		begin_test_suite( HierarchicalTableModelImplTests )

			shared_ptr< vector<organization> > underlying;
			organization_context ctx;

			init( SetData )
			{
				organization data_[] = {
					{	1, 0, "Microsoft", 100000	},
					{	2, 7, "VMware", 33000	},
					{	3, 6, "emc", 35000	},
					{	4, 0, "Apple", 95000	},
					{	5, 1, "Skype", 70000	},
					{	6, 7, "EMC", 33000	},
					{	7, 0, "Dell", 90000	},
				};

				underlying = make_vector(data_);
				ctx.by_id = [this] (unsigned id) {
					return 1 <= id && id <= underlying->size() ? &(*underlying)[id - 1] : nullptr;
				};
			}


			test( ModelIsOrderedAccordinglyToHierarchyAndTotalOrderInitially )
			{
				// INIT
				unsigned columns[] = {	0, 1,	};

				// INIT / ACT
				auto t = make_table(underlying, ctx);

				t->add_columns(create_columns());

				// ACT
				auto txt = get_text(*t, columns);

				// ASSERT
				string reference[][2] = {
					{	"Microsoft", "100000",	},
					{		"Skype", "70000",	},
					{	"Apple", "95000",	},
					{	"Dell", "90000",	},
					{		"VMware", "33000",	},
					{		"EMC", "33000",	},
					{			"emc", "35000",	},
				};

				assert_equal(mkvector(reference), txt);
			}


			test( HierarchicalSortingHonorsNesting )
			{
				// INIT
				unsigned columns[] = {	0, 1,	};

				// INIT / ACT
				auto t = make_table(underlying, ctx);

				t->add_columns(create_columns());

				// ACT
				t->set_order(0, true);
				auto txt = get_text(*t, columns);

				// ASSERT
				string reference1[][2] = {
					{	"Apple", "95000",	},
					{	"Dell", "90000",	},
					{		"EMC", "33000",	},
					{			"emc", "35000",	},
					{		"VMware", "33000",	},
					{	"Microsoft", "100000",	},
					{		"Skype", "70000",	},
				};

				assert_equal(mkvector(reference1), txt);

				// ACT
				t->set_order(0, false);
				txt = get_text(*t, columns);

				// ASSERT
				string reference2[][2] = {
					{	"Microsoft", "100000",	},
					{		"Skype", "70000",	},
					{	"Dell", "90000",	},
					{		"VMware", "33000",	},
					{		"EMC", "33000",	},
					{			"emc", "35000",	},
					{	"Apple", "95000",	},
				};

				assert_equal(mkvector(reference2), txt);
				
				// ACT
				t->set_order(1, true);
				txt = get_text(*t, columns);

				// ASSERT
				string reference3[][2] = {
					{	"Dell", "90000",	},
					{		"VMware", "33000",	},
					{		"EMC", "33000",	},
					{			"emc", "35000",	},
					{	"Apple", "95000",	},
					{	"Microsoft", "100000",	},
					{		"Skype", "70000",	},
				};

				assert_equal(mkvector(reference3), txt);
			}
		end_test_suite
	}
}
