#include <frontend/table_model_impl.h>

#include "helpers.h"

#include <common/formatting.h>
#include <frontend/helpers.h>
#include <wpl/models.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <typename T>
			class any_container : vector<T>
			{
			public:
				any_container()
				{	}

				template <typename U, size_t n>
				explicit any_container(U (&arr)[n])
					: vector<T>(arr, arr + n)
				{	}

				typedef typename vector<T>::const_iterator const_iterator;
				typedef typename vector<T>::const_reference const_reference;
				typedef typename vector<T>::value_type value_type;

				const_iterator begin() const
				{	return vector<T>::begin();	}

				const_iterator end() const
				{	return vector<T>::end();	}

				void _push_back(const T &value)
				{	this->push_back(value);	}
			};

			template <typename T>
			shared_ptr< table_model_impl<wpl::richtext_table_model, T, int> > mkmodel(shared_ptr<T> underlying)
			{	return make_shared< table_model_impl<wpl::richtext_table_model, T, int> >(underlying, 0);	}

			typedef pair<unsigned, unsigned> data1_t;
			typedef pair<unsigned, string> data2_t;
			typedef any_container<data1_t> underlying1_t;
			typedef any_container<data2_t> underlying2_t;

			template <typename T, typename CtxT, typename GetTextT>
			column_definition<T, CtxT> column(const GetTextT &get_text)
			{
				column_definition<T, CtxT> c = {	string(), agge::style_modifier::empty, 0, agge::align_near, get_text,	};
				return c;
			}

			template <typename T, typename CtxT, typename GetTextT, typename LessT>
			column_definition<T, CtxT> column(const GetTextT &get_text, const LessT &less_)
			{
				column_definition<T, CtxT> c = {	string(), agge::style_modifier::empty, 0, agge::align_near, get_text, less_,	};
				return c;
			}
		}

		begin_test_suite( TableModelImplTests )
			test( EmptyContainerMakesEmptyView )
			{
				// INIT
				auto underlying1 = make_shared<underlying1_t>();
				auto underlying2 = make_shared<underlying2_t>();

				// INIT / ACT
				auto v1 = mkmodel(underlying1);
				auto v2 = mkmodel(underlying2);

				// ACT / ASSERT
				assert_equal(0u, v1->get_count());
				assert_equal(0u, v2->get_count());

				// ACT
				v1->fetch();
				v2->fetch();

				// ACT / ASSERT
				assert_equal(0u, v1->get_count());
				assert_equal(0u, v2->get_count());
			}


			test( CountIsNotUpdatedBeforeFetch )
			{
				// INIT
				auto underlying = make_shared<underlying1_t>();
				auto v = mkmodel(underlying);

				underlying->_push_back(make_pair(1, 12));

				// ACT / ASSERT
				assert_equal(0u, v->get_count());

				// INIT
				underlying->_push_back(make_pair(11, 13));

				// ACT / ASSERT
				assert_equal(0u, v->get_count());
			}


			test( CountIsSetUponFetchDuringInvalidation )
			{
				// INIT
				auto underlying = make_shared<underlying1_t>();
				auto v = mkmodel(underlying);
				vector<wpl::richtext_table_model::index_type> log;
				auto conn = v->invalidate += [&] (wpl::richtext_table_model::index_type idx) {
					assert_equal(wpl::richtext_table_model::npos(), idx);
					log.push_back(v->get_count());
				};

				underlying->_push_back(make_pair(1, 12));

				// ACT
				v->fetch();

				// ASSERT
				wpl::richtext_table_model::index_type reference1[] = {
					1u,
				};

				assert_equal(reference1, log);

				// INIT
				underlying->_push_back(make_pair(2, 22));
				underlying->_push_back(make_pair(7, 12));

				// ACT
				v->fetch();

				// ASSERT
				wpl::richtext_table_model::index_type reference2[] = {
					1u, 3u,
				};

				assert_equal(reference2, log);
			}


			test( TextIsConvertedAccordinglyToAColumnConvertor )
			{
				// INIT
				data1_t data1[] = {
					make_pair(3, 1), make_pair(4, 1), make_pair(59, 26),
				};
				data2_t data2[] = {
					make_pair(3, "zoo"), make_pair(1, "Foo"), make_pair(41, "BAR"), make_pair(5, "z"),
				};
				auto underlying1 = make_shared<underlying1_t>();
				auto underlying2 = make_shared<underlying2_t>(data2);
				auto v1 = mkmodel(underlying1);
				auto v2 = mkmodel(underlying2);
				unsigned columns1[] = {	0, 1,	};
				unsigned columns2[] = {	0, 1, 2,	};

				*underlying1 = underlying1_t(data1);
				v1->fetch();

				// INIT / ACT
				v1->add_columns(plural
					+ column<data1_t, int>([] (agge::richtext_t &t, int, size_t, const data1_t &v) {	itoa<10>(t, v.first);	})
					+ column<data1_t, int>([] (agge::richtext_t &t, int, size_t, const data1_t &v) {	itoa<10>(t, v.second);	}));
				v2->add_columns(plural
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t, const data2_t &v) {	itoa<10>(t, v.first);	})
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t, const data2_t &v) {	t << v.second.c_str();	}));

				// ACT / ASSERT
				assert_equal(3u, v1->get_count());

				// ACT
				auto t = get_text(*v1, columns1);

				// ASSERT
				string reference1[][2] = {
					{	"3", "1",	}, {	"4", "1",	}, {	"59", "26",	},
				};

				assert_equal(mkvector(reference1), t);

				// ACT / ASSERT
				assert_equal(4u, v2->get_count());

				// ACT
				t = get_text(*v2, columns1);

				// ASSERT
				string reference2[][2] = {
					{	"3", "zoo",	}, {	"1", "Foo",	}, {	"41", "BAR",	}, {	"5", "z",	},
				};

				assert_equal(mkvector(reference2), t);

				// INIT / ACT
				v2->add_columns(plural
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t row, const data2_t &) {	itoa<10>(t, row);	}));

				// ACT
				t = get_text(*v2, columns2);

				// ASSERT
				string reference3[][3] = {
					{	"3", "zoo", "0",	}, {	"1", "Foo", "1",	}, {	"41", "BAR", "2",	}, {	"5", "z", "3",	},
				};

				assert_equal(mkvector(reference3), t);

				// INIT
				underlying2->_push_back(make_pair(7171, "zzZZzz"));

				// ACT
				v2->fetch();
				t = get_text(*v2, columns2);

				// ASSERT
				string reference4[][3] = {
					{	"3", "zoo", "0",	}, {	"1", "Foo", "1",	}, {	"41", "BAR", "2",	}, {	"5", "z", "3",	}, {	"7171", "zzZZzz", "4",	},
				};

				assert_equal(mkvector(reference4), t);
			}


			test( SortingIsAppliedAccordinglyToAColumnAndIsAvailableDuringInvalidation )
			{
				// INIT
				data2_t data[] = {
					make_pair(3, "zoo"), make_pair(1, "Foo"), make_pair(41, "BAR"), make_pair(5, "z"),
				};
				auto underlying = make_shared<underlying2_t>(data);
				auto v = mkmodel(underlying);
				unsigned columns[] = {	0,	};
				vector< vector< vector<string> > > log;

				*underlying = underlying2_t(data);
				v->fetch();

				// INIT / ACT
				v->add_columns(plural
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t, const data2_t &v) {	itoa<10>(t, v.first);	},
						[] (int, const data2_t &lhs, const data2_t &rhs) {	return compare(lhs.first, rhs.first);	})
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t, const data2_t &v) {	t << v.second.c_str();	},
						[] (int, const data2_t &lhs, const data2_t &rhs) {	return compare(lhs.second, rhs.second);	})
					+ column<data2_t, int>([] (agge::richtext_t &t, int, size_t row, const data2_t &) {	itoa<10>(t, row);	}));

				auto conn = v->invalidate += [&] (wpl::richtext_table_model::index_type idx) {
					log.push_back(get_text(*v, columns));
					assert_equal(wpl::richtext_table_model::npos(), idx);
				};

				// ACT
				v->set_order(0, true);

				// ASSERT
				string reference1[][1] = {
					{	"1",	}, {	"3",	}, {	"5",	}, {	"41",	},
				};

				assert_equal(1u, log.size());
				assert_equal(mkvector(reference1), log.back());

				// ACT
				v->set_order(0, false);

				// ASSERT
				string reference2[][1] = {
					{	"41",	}, {	"5",	}, {	"3",	}, {	"1",	},
				};

				assert_equal(2u, log.size());
				assert_equal(mkvector(reference2), log.back());

				// ACT
				v->set_order(1, true);

				// ASSERT
				string reference3[][1] = {
					{	"41",	}, {	"1",	}, {	"5",	}, {	"3",	},
				};

				assert_equal(3u, log.size());
				assert_equal(mkvector(reference3), log.back());

				// ACT
				v->set_order(2, true);

				// ASSERT
				assert_equal(3u, log.size());

				// ACT
				v->set_order(2, false);

				// ASSERT
				assert_equal(3u, log.size());

				// ACT / ASSERT
				assert_throws(v->set_order(3 /*out of range*/, false), out_of_range);
			}


			test( GettingTextForMissingFieldsLeavesTextUnmodified )
			{
				// INIT
				data2_t data[] = {
					make_pair(3, "zoo"), make_pair(1, "Foo"), make_pair(41, "BAR"), make_pair(5, "z"),
				};
				auto underlying = make_shared<underlying2_t>(data);
				auto v = mkmodel(underlying);
				agge::richtext_t text((agge::font_style_annotation()));

				text << "test";

				// ACT / ASSERT (no exceptions)
				v->get_text(0, 0, text);

				// ASSERT
				assert_equal("test", text.underlying());

				// INIT
				text << "z";

				// ACT / ASSERT (no exceptions)
				v->get_text(0, 1, text);
				v->get_text(1, 0, text);

				// ASSERT
				assert_equal("testz", text.underlying());

				// INIT
				v->add_columns(plural
					+ column<data2_t, int>([] (agge::richtext_t &text, int, size_t, const data2_t &v) {	text << v.second.c_str();	}));

				// ACT
				v->get_text(1, 0, text);
				v->get_text(1, 1, text);

				// ASSERT
				assert_equal("testzFoo", text.underlying());
			}


			test( ContextIsPassedToAccessorsAndPredicates )
			{
				// INIT
				pair<int, int> data_[] = {	make_pair(3, 3), make_pair(3, 7),	};
				auto u = make_shared<underlying1_t>(data_);
				auto m1 = make_shared< table_model_impl<wpl::richtext_table_model, underlying1_t, double> >(u, 0.1231);
				auto m2 = make_shared< table_model_impl<wpl::richtext_table_model, underlying1_t, double> >(u, 3.111891);
				auto m3 = make_shared< table_model_impl<wpl::richtext_table_model, underlying1_t, string> >(u, "lorem ipsum");
				double ctx1;
				string ctx2;
				agge::richtext_t txt((agge::font_style_annotation()));

				m1->add_columns(plural
					+ column<data1_t, double>([&ctx1] (agge::richtext_t &, double c, size_t, data1_t) {	ctx1 = c;	},
						[&ctx1] (double c, data1_t, data1_t) {	return ctx1 = c, 0;	}));
				m2->add_columns(plural
					+ column<data1_t, double>([&ctx1] (agge::richtext_t &, double c, size_t, data1_t) {	ctx1 = c;	},
						[&ctx1] (double c, data1_t, data1_t) {	return ctx1 = c, 0;	}));
				m3->add_columns(plural
					+ column<data1_t, string>([&ctx2] (agge::richtext_t &, const string &c, size_t, data1_t) {	ctx2 = c;	},
						[&ctx2] (const string &c, data1_t, data1_t) {	return ctx2 = c, 0;	}));

				// ACT
				m1->get_text(0, 0, txt);
				m3->get_text(0, 0, txt);

				// ASSERT
				assert_equal(0.1231, ctx1);
				assert_equal("lorem ipsum", ctx2);

				// ACT
				m2->get_text(0, 0, txt);

				// ASSERT
				assert_equal(3.111891, ctx1);

				// INIT
				ctx1 = 0;
				ctx2 = "";

				// ACT
				m1->set_order(0, true);
				m3->set_order(0, true);

				// ASSERT
				assert_equal(0.1231, ctx1);
				assert_equal("lorem ipsum", ctx2);

				// ACT
				m2->set_order(0, true);

				// ASSERT
				assert_equal(3.111891, ctx1);
			}
		end_test_suite
	}
}
