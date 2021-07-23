#include <frontend/headers_model.h>

#include <common/configuration.h>
#include <map>
#include <test-helpers/richtext.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace agge;
using namespace std;
using namespace std::placeholders;

namespace agge
{
	inline bool operator ==(const full_alignment &lhs, const full_alignment &rhs)
	{	return lhs.halign == rhs.halign && lhs.valign == rhs.valign;	}
}

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef vector< pair<headers_model::index_type, bool> > log_t;

			void append_log(log_t *log, headers_model::index_type sort_column, bool sort_ascending)
			{	log->push_back(make_pair(sort_column, sort_ascending));	}

			short int get_width(const wpl::headers_model &cm, headers_model::index_type item)
			{
				short int v;

				return cm.get_value(item, v), v;
			}

			class mock_hive : public hive
			{
			public:
				typedef map<string, int> int_values_map;
				typedef map<string, string> str_values_map;
				
			public:
				mock_hive(int_values_map &int_values, str_values_map &str_values, const char *prefix = "");

			private:
				const mock_hive &operator =(const mock_hive &rhs);

				virtual shared_ptr<hive> create(const char *name);
				virtual shared_ptr<const hive> open(const char *name) const;

				virtual void store(const char *name, int value);
				virtual void store(const char *name, const char *value);

				virtual bool load(const char *name, int &value) const;
				virtual bool load(const char *name, string &value) const;

			private:
				int_values_map &_int_values;
				str_values_map &_str_values;
				const string _prefix;
			};


			mock_hive::mock_hive(int_values_map &int_values, str_values_map &str_values, const char *prefix)
				: _int_values(int_values), _str_values(str_values), _prefix(prefix)
			{	}

			shared_ptr<hive> mock_hive::create(const char *name)
			{	return shared_ptr<hive>(new mock_hive(_int_values, _str_values, (_prefix + name + "/").c_str()));	}

			template <typename T>
			bool contains_prefix(const map<string, T> &storage, const string &prefix)
			{
				return 0 != distance(storage.lower_bound(prefix),
					storage.lower_bound(prefix.substr(0, prefix.size() - 1) + static_cast<char>('/' + 1)));
			}

			shared_ptr<const hive> mock_hive::open(const char *name) const
			{
				const string prefix = _prefix + name + "/";
				
				if (contains_prefix(_int_values, prefix) || contains_prefix(_str_values, prefix))
					return shared_ptr<hive>(new mock_hive(_int_values, _str_values, prefix.c_str()));
				else
					return shared_ptr<hive>();
			}

			void mock_hive::store(const char *name, int value)
			{	_int_values[_prefix + name] = value;	}

			void mock_hive::store(const char *name, const char *value)
			{	_str_values[_prefix + name] = value;	}

			bool mock_hive::load(const char *name, int &value) const
			{
				int_values_map::const_iterator m = _int_values.find(_prefix + name);

				return m != _int_values.end() ? value = m->second, true : false;
			}

			bool mock_hive::load(const char *name, string &value) const
			{
				str_values_map::const_iterator m = _str_values.find(_prefix + name);

				return m != _str_values.end() ? value = m->second, true : false;
			}
		}

		begin_test_suite( HeadersModelTests )
			test( ColumnsModelIsInitiallyOrderedAcordinglyToConstructionParam )
			{
				// INIT
				headers_model::column columns[] = {
					{	"id1", T(""), 0, headers_model::dir_none	},
					{	"id2", T(""), 0, headers_model::dir_descending	},
					{	"id3", T(""), 0, headers_model::dir_descending	},
				};

				// ACT
				shared_ptr<headers_model> cm1(new headers_model(columns, headers_model::npos(),
					false));

				// ACT / ASSERT
				assert_equal(headers_model::npos(), cm1->get_sort_order().first);
				assert_is_false(cm1->get_sort_order().second);

				// ACT
				shared_ptr<headers_model> cm2(new headers_model(columns, 1, false));

				// ACT / ASSERT
				assert_equal(1u, cm2->get_sort_order().first);
				assert_is_false(cm2->get_sort_order().second);

				// ACT
				shared_ptr<headers_model> cm3(new headers_model(columns, 2, true));

				// ACT / ASSERT
				assert_equal(2u, cm3->get_sort_order().first);
				assert_is_true(cm3->get_sort_order().second);
			}


			test( FirstOrderingIsDoneAccordinglyToDefaults )
			{
				// INIT
				headers_model::column columns[] = {
					{	"id1", T(""), 0, headers_model::dir_none	},
					{	"id2", T(""), 0, headers_model::dir_descending	},
					{	"id3", T(""), 0, headers_model::dir_ascending	},
					{	"id4", T(""), 0, headers_model::dir_descending	},
				};
				shared_ptr<headers_model> cm;
				log_t log;
				wpl::slot_connection slot;

				// ACT
				cm.reset(new headers_model(columns, headers_model::npos(), false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(0);

				// ACT / ASSERT
				assert_is_empty(log);
				assert_is_false(cm->get_sort_order().second);
				assert_equal(headers_model::npos(), cm->get_sort_order().first);

				// ACT
				cm.reset(new headers_model(columns, headers_model::npos(), false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(3);

				// ACT / ASSERT
				assert_equal(3u, cm->get_sort_order().first);
				assert_is_false(cm->get_sort_order().second);
				assert_equal(1u, log.size());
				assert_equal(3u, log[0].first);
				assert_is_false(log[0].second);

				// ACT
				cm.reset(new headers_model(columns, headers_model::npos(), false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(2);

				// ACT / ASSERT
				assert_equal(2u, cm->get_sort_order().first);
				assert_is_true(cm->get_sort_order().second);
				assert_equal(2u, log.size());
				assert_equal(2u, log[1].first);
				assert_is_true(log[1].second);
			}


			test( SubsequentOrderingReversesTheColumnsState )
			{
				// INIT
				headers_model::column columns[] = {
					{	"id1", T(""), 0, headers_model::dir_none	},
					{	"id2", T(""), 0, headers_model::dir_descending	},
					{	"id3", T(""), 0, headers_model::dir_ascending	},
					{	"id4", T(""), 0, headers_model::dir_descending	},
				};
				shared_ptr<headers_model> cm1(new headers_model(columns, headers_model::npos(),
					false));
				shared_ptr<headers_model> cm2(new headers_model(columns, 2, false));
				log_t log;
				wpl::slot_connection slot1, slot2;

				slot1 = cm1->sort_order_changed += bind(&append_log, &log, _1, _2);
				slot2 = cm2->sort_order_changed += bind(&append_log, &log, _1, _2);

				// ACT
				cm1->activate_column(1);	// desending
				cm1->activate_column(1);	// ascending

				// ACT / ASSERT
				assert_equal(1u, cm1->get_sort_order().first);
				assert_is_true(cm1->get_sort_order().second);
				assert_equal(2u, log.size());
				assert_equal(1u, log[1].first);
				assert_is_true(log[1].second);

				// ACT
				cm2->activate_column(2);	// ascending

				// ACT / ASSERT
				assert_equal(2u, cm2->get_sort_order().first);
				assert_is_true(cm2->get_sort_order().second);
				assert_equal(3u, log.size());
				assert_equal(2u, log[2].first);
				assert_is_true(log[1].second);
			}


			test( GettingColumnsCount )
			{
				// INIT
				headers_model::column columns1[] = {
					{	"id1", T("first"), 0, headers_model::dir_none	},
					{	"id2", T("second"), 0, headers_model::dir_descending	},
					{	"id3", T("third"), 0, headers_model::dir_ascending	},
					{	"id4", T("fourth"), 0, headers_model::dir_ascending	},
				};
				headers_model::column columns2[] = {
					{	"id1", T("a first column"), 0, headers_model::dir_none	},
					{	"id2", T("a second column"), 0, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm1(new headers_model(columns1, 0, false));
				shared_ptr<headers_model> cm2(new headers_model(columns2, 0, false));

				// ACT / ASSERT
				assert_equal(4u, cm1->get_count());
				assert_equal(2u, cm2->get_count());
			}


			test( GetColumnItem )
			{
				// INIT
				headers_model::column columns1[] = {
					{	"id1", "first" + style::weight(regular), 0, headers_model::dir_none, agge::align_near	},
					{	"id2", "second" + style::weight(semi_bold) + "appendix", 0, headers_model::dir_descending, agge::align_far	},
					{	"id3", "third" + style::weight(regular), 0, headers_model::dir_ascending, agge::align_center	},
				};
				headers_model::column columns2[] = {
					{	"id1", "a first " + style::family("verdana") + "column", 0, headers_model::dir_none	},
					{	"id2", "a second column" + style::weight(regular), 0, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm1(new headers_model(columns1, 0, false));
				shared_ptr<headers_model> cm2(new headers_model(columns2, 0, false));
				font_style_annotation a = {	font_descriptor::create("Tahoma", 10)	};
				richtext_t caption(a);
				auto a2 = a;
				auto a3 = a;

				a2.basic.weight = semi_bold;
				a3.basic.family = "verdana";

				// ACT
				cm1->get_caption(0, caption);

				// ASSERT
				assert_equal(1, distance(caption.ranges_begin(), caption.ranges_end()));
				richtext_t::const_iterator i = caption.ranges_begin();
				assert_equal("first", string(i->begin(), i->end()));
				assert_equal(a.basic, i->get_annotation().basic);

				assert_equal(agge::full_alignment::create(agge::align_near, agge::align_near), cm1->get_header_alignment(0));
				assert_equal(agge::full_alignment::create(agge::align_near, agge::align_center), cm1->get_alignment(0));
				assert_equal(agge::full_alignment::create(agge::align_far, agge::align_near), cm1->get_header_alignment(1));
				assert_equal(agge::full_alignment::create(agge::align_far, agge::align_center), cm1->get_alignment(1));
				assert_equal(agge::full_alignment::create(agge::align_center, agge::align_near), cm1->get_header_alignment(2));
				assert_equal(agge::full_alignment::create(agge::align_center, agge::align_center), cm1->get_alignment(2));

				// ACT
				caption.clear();
				cm1->get_caption(1, caption);

				// ASSERT
				assert_equal(2, distance(caption.ranges_begin(), caption.ranges_end()));
				i = caption.ranges_begin();
				assert_equal("second", string(i->begin(), i->end()));
				assert_equal(a.basic, i->get_annotation().basic);
				++i;
				assert_equal("appendix", string(i->begin(), i->end()));
				assert_equal(a2.basic, i->get_annotation().basic);

				// ACT
				caption.clear();
				cm1->get_caption(2, caption);

				// ASSERT
				assert_equal(1, distance(caption.ranges_begin(), caption.ranges_end()));
				i = caption.ranges_begin();
				assert_equal("third", string(i->begin(), i->end()));
				assert_equal(a.basic, i->get_annotation().basic);

				// ACT
				caption.clear();
				cm2->get_caption(0, caption);

				// ASSERT
				assert_equal(2, distance(caption.ranges_begin(), caption.ranges_end()));
				i = caption.ranges_begin();
				assert_equal("a first ", string(i->begin(), i->end()));
				assert_equal(a.basic, i->get_annotation().basic);
				++i;
				assert_equal("column", string(i->begin(), i->end()));
				assert_equal(a3.basic, i->get_annotation().basic);

				// ACT
				caption.clear();
				cm2->get_caption(1, caption);

				// ASSERT
				assert_equal(1, distance(caption.ranges_begin(), caption.ranges_end()));
				i = caption.ranges_begin();
				assert_equal("a second column", string(i->begin(), i->end()));
				assert_equal(a.basic, i->get_annotation().basic);
			}


			test( ModelUpdateDoesChangeColumnsWidth )
			{
				// INIT
				headers_model::column columns[] = {
					{	"id1", T("first"), 0, headers_model::dir_none	},
					{	"id2", T("second"), 0, headers_model::dir_descending	},
					{	"id3", T("third"), 0, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm(new headers_model(columns, 0, false));

				// ACT
				cm->set_width(0, 13);

				// ASSERT
				assert_equal(13, get_width(*cm, 0));
				assert_equal(0, get_width(*cm, 1));
				assert_equal(0, get_width(*cm, 2));

				// ACT
				cm->set_width(1, 17);

				// ASSERT
				assert_equal(13, get_width(*cm, 0));
				assert_equal(17, get_width(*cm, 1));
				assert_equal(0, get_width(*cm, 2));
			}


			test( ModelUpdateInvalidatesIt )
			{
				// INIT
				vector<headers_model::index_type> invalidations;
				headers_model::column columns[] = {
					{	"id1", T("first"), 0, headers_model::dir_none	},
					{	"id2", T("second"), 0, headers_model::dir_descending	},
					{	"id3", T("third"), 0, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm(new headers_model(columns, 0, false));
				auto c = cm->invalidate += [&] (headers_model::index_type item) {	invalidations.push_back(item);	};

				// ACT
				cm->set_width(0, 15);

				// ASSERT
				headers_model::index_type reference1[] = {	0u,	};

				assert_equal(reference1, invalidations);

				// ACT
				cm->set_width(2, 3);
				cm->set_width(1, 13);

				// ASSERT
				headers_model::index_type reference2[] = {	0u, 2u, 1u,	};

				assert_equal(reference2, invalidations);
			}


			test( UnchangedModelIsStoredAsProvidedAtConstruction )
			{
				// INIT
				headers_model::column columns1[] = {
					{	"id1", T("Index"), 1, headers_model::dir_none	},
					{	"id2", T("Function"), 2, headers_model::dir_descending	},
					{	"id3", T("Exclusive Time"), 3, headers_model::dir_ascending	},
					{	"fourth", T("Inclusive Time"), 4, headers_model::dir_ascending	},
				};
				headers_model::column columns2[] = {
					{	"id1", T("a first column"), 191, headers_model::dir_none	},
					{	"id2", T("a second column"), 171, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm1(new headers_model(columns1, 0, false));
				shared_ptr<headers_model> cm2(new headers_model(columns2, headers_model::npos(), false));
				shared_ptr<headers_model> cm3(new headers_model(columns2, 1, true));

				map<string, int> int_values;
				map<string, string> str_values;
				shared_ptr<mock_hive> h(new mock_hive(int_values, str_values));

				// ACT
				cm1->store(*h);

				// ASSERT
				pair<const string, int> rints1[] = {
					make_pair("OrderBy", 0),
					make_pair("OrderDirection", 0),
					make_pair("fourth/Width", 4),
					make_pair("id1/Width", 1),
					make_pair("id2/Width", 2),
					make_pair("id3/Width", 3),
				};
//				pair<const string, string> rstrings1[] = {
//					make_pair("fourth/Caption", "Inclusive Time"),
//					make_pair("id1/Caption", "Index"),
//					make_pair("id2/Caption", "Function"),
//					make_pair("id3/Caption", "Exclusive Time"),
//				};

				assert_equal(rints1, int_values);
//				assert_equal(rstrings1, str_values);

				// INIT
				int_values.clear();
				str_values.clear();

				// ACT
				cm2->store(*h);

				// ASSERT
				pair<const string, int> rints2[] = {
					make_pair("OrderBy", -1),
					make_pair("OrderDirection", 0),
					make_pair("id1/Width", 191),
					make_pair("id2/Width", 171),
				};
//				pair<const string, string> rstrings2[] = {
//					make_pair("id1/Caption", "a first column"),
//					make_pair("id2/Caption", "a second column"),
//				};

				assert_equal(rints2, int_values);
//				assert_equal(rstrings2, str_values);

				// INIT
				int_values.clear();
				str_values.clear();

				// ACT
				cm3->store(*h);

				// ASSERT
				pair<const string, int> rints3[] = {
					make_pair("OrderBy", 1),
					make_pair("OrderDirection", 1),
					make_pair("id1/Width", 191),
					make_pair("id2/Width", 171),
				};

				assert_equal(rints3, int_values);
//				assert_equal(rstrings2, str_values);
			}


			test( ModelStateIsUpdatedOnLoad )
			{
				// INIT
				headers_model::column columns1[] = {
					{	"id1", T(""), 1, headers_model::dir_none	},
					{	"id2", T(""), 2, headers_model::dir_ascending	},
				};
				headers_model::column columns2[] = {
					{	"col1", T(""), 0, headers_model::dir_none	},
					{	"col2", T(""), 0, headers_model::dir_ascending	},
					{	"col3", T(""), 0, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm1(new headers_model(columns1, 0, false));
				shared_ptr<headers_model> cm2(new headers_model(columns2, 0, false));

				map<string, int> int_values;
				map<string, string> str_values;
				shared_ptr<mock_hive> h(new mock_hive(int_values, str_values));

				int_values["OrderBy"] = 1;
				int_values["OrderDirection"] = 1;
				str_values["id1/Caption"] = "Contract";
				int_values["id1/Width"] = 20;
				str_values["id2/Caption"] = "Price";
				int_values["id2/Width"] = 31;

				// ACT
				cm1->update(*h);

				// ASSERT
				assert_equal(make_pair(static_cast<headers_model::index_type>(1), true), cm1->get_sort_order());
				assert_equal(20, get_width(*cm1, 0));
				assert_equal(31, get_width(*cm1, 1));

				// INIT
				int_values["OrderBy"] = 2;
				int_values["OrderDirection"] = 0;
				str_values["col1/Caption"] = "Contract";
				int_values["col1/Width"] = 20;
				str_values["col2/Caption"] = "Kind";
				int_values["col2/Width"] = 20;
				str_values["col3/Caption"] = "Price";
				int_values["col3/Width"] = 53;

				// ACT
				cm2->update(*h);

				// ASSERT
				assert_equal(make_pair(static_cast<headers_model::index_type>(2), false), cm2->get_sort_order());
				assert_equal(20, get_width(*cm2, 0));
				assert_equal(20, get_width(*cm2, 1));
				assert_equal(53, get_width(*cm2, 2));
			}


			test( MissingColumnsAreNotUpdatedAndInvalidOrderColumnIsFixedOnModelStateLoad )
			{
				// INIT
				headers_model::column columns[] = {
					{	"col1", T(""), 13, headers_model::dir_none	},
					{	"col2", T(""), 17, headers_model::dir_ascending	},
					{	"col3", T(""), 31, headers_model::dir_ascending	},
				};
				shared_ptr<headers_model> cm(new headers_model(columns, 0, false));

				map<string, int> int_values;
				map<string, string> str_values;
				shared_ptr<mock_hive> h(new mock_hive(int_values, str_values));

				int_values["OrderBy"] = 3;
				int_values["OrderDirection"] = 0;
				str_values["col1/Caption"] = "Contract";
				int_values["col1/Width"] = 233;
				str_values["col3/Caption"] = "Price";

				// ACT
				cm->update(*h);

				// ASSERT
				assert_equal(make_pair(headers_model::npos(), false), cm->get_sort_order());
				assert_equal(233, get_width(*cm, 0));
				assert_equal(17, get_width(*cm, 1));
				assert_equal(31, get_width(*cm, 2));
			}
		end_test_suite
	}
}
