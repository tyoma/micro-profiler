#include <frontend/columns_model.h>

#include "../assert.h"

#include <common/configuration.h>
#include <map>

namespace std
{
	using tr1::shared_ptr;
	using tr1::enable_shared_from_this;
	using tr1::bind;
   using namespace tr1::placeholders;
}

using namespace std;
using namespace wpl::ui;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			typedef vector< pair<listview::columns_model::index_type, bool> > log_t;

			void append_log(log_t *log, listview::columns_model::index_type sort_column, bool sort_ascending)
			{	log->push_back(make_pair(sort_column, sort_ascending));	}

			bool operator ==(const wpl::ui::listview::columns_model::column &lhs,
				const wpl::ui::listview::columns_model::column &rhs)
			{	return lhs.caption == rhs.caption && lhs.width == rhs.width;	}

			wpl::ui::listview::columns_model::column get_column(const wpl::ui::listview::columns_model &cm,
				columns_model::index_type index)
			{
				wpl::ui::listview::columns_model::column c;

				cm.get_column(index, c);
				return c;
			}

			class mock_hive : public hive
			{
			public:
				typedef map<string, int> int_values_map;
				typedef map<string, wstring> str_values_map;
				
			public:
				mock_hive(int_values_map &int_values, str_values_map &str_values, const char *prefix = "");

			private:
				const mock_hive &operator =(const mock_hive &rhs);

				virtual shared_ptr<hive> create(const char *name);
				virtual shared_ptr<const hive> open(const char *name) const;

				virtual void store(const char *name, int value);
				virtual void store(const char *name, const wchar_t *value);

				virtual bool load(const char *name, int &value) const;
				virtual bool load(const char *name, wstring &value) const;

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

			void mock_hive::store(const char *name, const wchar_t *value)
			{	_str_values[_prefix + name] = value;	}

			bool mock_hive::load(const char *name, int &value) const
			{
				int_values_map::const_iterator m = _int_values.find(_prefix + name);

				return m != _int_values.end() ? value = m->second, true : false;
			}

			bool mock_hive::load(const char *name, wstring &value) const
			{
				str_values_map::const_iterator m = _str_values.find(_prefix + name);

				return m != _str_values.end() ? value = m->second, true : false;
			}
		}

		[TestClass]
		public ref class ColumnsModelTests
		{
		public:
			[TestMethod]
			void ColumnsModelIsInitiallyOrderedAcordinglyToConstructionParam()
			{
				// INIT
				columns_model::column columns[] = {
					columns_model::column("id1", L"", 0, columns_model::dir_none),
					columns_model::column("id2", L"", 0, columns_model::dir_descending),
					columns_model::column("id3", L"", 0, columns_model::dir_descending),
				};

				// ACT
				shared_ptr<listview::columns_model> cm1(new columns_model(columns, listview::columns_model::npos,
					false));

				// ACT / ASSERT
				Assert::IsTrue(listview::columns_model::npos == cm1->get_sort_order().first);
				Assert::IsFalse(cm1->get_sort_order().second);

				// ACT
				shared_ptr<listview::columns_model> cm2(new columns_model(columns, 1, false));

				// ACT / ASSERT
				Assert::IsTrue(1 == cm2->get_sort_order().first);
				Assert::IsFalse(cm2->get_sort_order().second);

				// ACT
				shared_ptr<listview::columns_model> cm3(new columns_model(columns, 2, true));

				// ACT / ASSERT
				Assert::IsTrue(2 == cm3->get_sort_order().first);
				Assert::IsTrue(cm3->get_sort_order().second);
			}


			[TestMethod]
			void FirstOrderingIsDoneAccordinglyToDefaults()
			{
				// INIT
				columns_model::column columns[] = {
					columns_model::column("id1", L"", 0, columns_model::dir_none),
					columns_model::column("id2", L"", 0, columns_model::dir_descending),
					columns_model::column("id3", L"", 0, columns_model::dir_ascending),
					columns_model::column("id4", L"", 0, columns_model::dir_descending),
				};
				shared_ptr<listview::columns_model> cm;
				log_t log;
				shared_ptr<wpl::destructible> slot;

				// ACT
				cm.reset(new columns_model(columns, listview::columns_model::npos, false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(0);

				// ACT / ASSERT
				Assert::IsTrue(log.empty());
				Assert::IsFalse(cm->get_sort_order().second);
				Assert::IsTrue(listview::columns_model::npos == cm->get_sort_order().first);

				// ACT
				cm.reset(new columns_model(columns, listview::columns_model::npos, false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(3);

				// ACT / ASSERT
				Assert::IsTrue(3 == cm->get_sort_order().first);
				Assert::IsFalse(cm->get_sort_order().second);
				Assert::IsTrue(1 == log.size());
				Assert::IsTrue(3 == log[0].first);
				Assert::IsFalse(log[0].second);

				// ACT
				cm.reset(new columns_model(columns, listview::columns_model::npos, false));
				slot = cm->sort_order_changed += bind(&append_log, &log, _1, _2);
				cm->activate_column(2);

				// ACT / ASSERT
				Assert::IsTrue(2 == cm->get_sort_order().first);
				Assert::IsTrue(cm->get_sort_order().second);
				Assert::IsTrue(2 == log.size());
				Assert::IsTrue(2 == log[1].first);
				Assert::IsTrue(log[1].second);
			}


			[TestMethod]
			void SubsequentOrderingReversesTheColumnsState()
			{
				// INIT
				columns_model::column columns[] = {
					columns_model::column("id1", L"", 0, columns_model::dir_none),
					columns_model::column("id2", L"", 0, columns_model::dir_descending),
					columns_model::column("id3", L"", 0, columns_model::dir_ascending),
					columns_model::column("id4", L"", 0, columns_model::dir_descending),
				};
				shared_ptr<listview::columns_model> cm1(new columns_model(columns, listview::columns_model::npos,
					false));
				shared_ptr<listview::columns_model> cm2(new columns_model(columns, 2, false));
				log_t log;
				shared_ptr<wpl::destructible> slot1, slot2;

				slot1 = cm1->sort_order_changed += bind(&append_log, &log, _1, _2);
				slot2 = cm2->sort_order_changed += bind(&append_log, &log, _1, _2);

				// ACT
				cm1->activate_column(1);	// desending
				cm1->activate_column(1);	// ascending

				// ACT / ASSERT
				Assert::IsTrue(1 == cm1->get_sort_order().first);
				Assert::IsTrue(cm1->get_sort_order().second);
				Assert::IsTrue(2 == log.size());
				Assert::IsTrue(1 == log[1].first);
				Assert::IsTrue(log[1].second);

				// ACT
				cm2->activate_column(2);	// ascending

				// ACT / ASSERT
				Assert::IsTrue(2 == cm2->get_sort_order().first);
				Assert::IsTrue(cm2->get_sort_order().second);
				Assert::IsTrue(3 == log.size());
				Assert::IsTrue(2 == log[2].first);
				Assert::IsTrue(log[1].second);
			}


			[TestMethod]
			void GettingColumnsCount()
			{
				// INIT
				columns_model::column columns1[] = {
					columns_model::column("id1", L"first", 0, columns_model::dir_none),
					columns_model::column("id2", L"second", 0, columns_model::dir_descending),
					columns_model::column("id3", L"third", 0, columns_model::dir_ascending),
					columns_model::column("id4", L"fourth", 0, columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", 0, columns_model::dir_none),
					columns_model::column("id2", L"a second column", 0, columns_model::dir_ascending),
				};
				shared_ptr<listview::columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<listview::columns_model> cm2(new columns_model(columns2, 0, false));
				listview::columns_model::column c;

				// ACT / ASSERT
				Assert::IsTrue(4 == cm1->get_count());
				Assert::IsTrue(2 == cm2->get_count());
			}


			[TestMethod]
			void GetColumnItem()
			{
				// INIT
				columns_model::column columns1[] = {
					columns_model::column("id1", L"first", 0, columns_model::dir_none),
					columns_model::column("id2", L"second", 0, columns_model::dir_descending),
					columns_model::column("id3", L"third", 0, columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", 0, columns_model::dir_none),
					columns_model::column("id2", L"a second column", 0, columns_model::dir_ascending),
				};
				shared_ptr<listview::columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<listview::columns_model> cm2(new columns_model(columns2, 0, false));
				listview::columns_model::column c;

				// ACT
				cm1->get_column(0, c);

				// ASSERT
				Assert::IsTrue(L"first" == c.caption);

				// ACT
				cm1->get_column(2, c);

				// ASSERT
				Assert::IsTrue(L"third" == c.caption);

				// ACT
				cm2->get_column(0, c);

				// ASSERT
				Assert::IsTrue(L"a first column" == c.caption);

				// ACT
				cm2->get_column(1, c);

				// ASSERT
				Assert::IsTrue(L"a second column" == c.caption);
			}


			[TestMethod]
			void ModelUpdateDoesChangeColumnsWidth()
			{
				// INIT
				columns_model::column columns[] = {
					columns_model::column("id1", L"first", 0, columns_model::dir_none),
					columns_model::column("id2", L"second", 0, columns_model::dir_descending),
					columns_model::column("id3", L"third", 0, columns_model::dir_ascending),
				};
				shared_ptr<listview::columns_model> cm(new columns_model(columns, 0, false));
				listview::columns_model::column c;

				// ACT
				cm->update_column(0, 13);

				// ASSERT
				Assert::IsTrue(13 == get_column(*cm, 0).width);
				Assert::IsTrue(0 == get_column(*cm, 1).width);
				Assert::IsTrue(0 == get_column(*cm, 2).width);

				// ACT
				cm->update_column(1, 17);

				// ASSERT
				Assert::IsTrue(13 == get_column(*cm, 0).width);
				Assert::IsTrue(17 == get_column(*cm, 1).width);
				Assert::IsTrue(0 == get_column(*cm, 2).width);
			}


			[TestMethod]
			void UnchangedModelIsStoredAsProvidedAtConstruction()
			{
				// INIT
				columns_model::column columns1[] = {
					columns_model::column("id1", L"Index", 1, columns_model::dir_none),
					columns_model::column("id2", L"Function", 2, columns_model::dir_descending),
					columns_model::column("id3", L"Exclusive Time", 3, columns_model::dir_ascending),
					columns_model::column("fourth", L"Inclusive Time", 4, columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", 191, columns_model::dir_none),
					columns_model::column("id2", L"a second column", 171, columns_model::dir_ascending),
				};
				shared_ptr<columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<columns_model> cm2(new columns_model(columns2, listview::columns_model::npos, false));
				shared_ptr<columns_model> cm3(new columns_model(columns2, 1, true));

				map<string, int> int_values;
				map<string, wstring> str_values;
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
				pair<const string, wstring> rstrings1[] = {
					make_pair("fourth/Caption", L"Inclusive Time"),
					make_pair("id1/Caption", L"Index"),
					make_pair("id2/Caption", L"Function"),
					make_pair("id3/Caption", L"Exclusive Time"),
				};

				assert::sequences_equal(rints1, int_values);
				assert::sequences_equal(rstrings1, str_values);

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
				pair<const string, wstring> rstrings2[] = {
					make_pair("id1/Caption", L"a first column"),
					make_pair("id2/Caption", L"a second column"),
				};

				assert::sequences_equal(rints2, int_values);
				assert::sequences_equal(rstrings2, str_values);

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

				assert::sequences_equal(rints3, int_values);
				assert::sequences_equal(rstrings2, str_values);
			}


			[TestMethod]
			void ModelStateIsUpdatedOnLoad()
			{
				// INIT
				columns_model::column columns1[] = {
					columns_model::column("id1", L"", 1, columns_model::dir_none),
					columns_model::column("id2", L"", 2, columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("col1", L"", 0, columns_model::dir_none),
					columns_model::column("col2", L"", 0, columns_model::dir_ascending),
					columns_model::column("col3", L"", 0, columns_model::dir_ascending),
				};
				shared_ptr<columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<columns_model> cm2(new columns_model(columns2, 0, false));

				map<string, int> int_values;
				map<string, wstring> str_values;
				shared_ptr<mock_hive> h(new mock_hive(int_values, str_values));

				int_values["OrderBy"] = 1;
				int_values["OrderDirection"] = 1;
				str_values["id1/Caption"] = L"Contract";
				int_values["id1/Width"] = 20;
				str_values["id2/Caption"] = L"Price";
				int_values["id2/Width"] = 31;

				// ACT
				cm1->update(*h);

				// ASSERT
				Assert::IsTrue(make_pair(static_cast<short>(1), true) == cm1->get_sort_order());
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Contract", 20) == get_column(*cm1, 0));
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Price", 31) == get_column(*cm1, 1));

				// INIT
				int_values["OrderBy"] = 2;
				int_values["OrderDirection"] = 0;
				str_values["col1/Caption"] = L"Contract";
				int_values["col1/Width"] = 20;
				str_values["col2/Caption"] = L"Kind";
				int_values["col2/Width"] = 20;
				str_values["col3/Caption"] = L"Price";
				int_values["col3/Width"] = 53;

				// ACT
				cm2->update(*h);

				// ASSERT
				Assert::IsTrue(make_pair(static_cast<short>(2), false) == cm2->get_sort_order());
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Contract", 20) == get_column(*cm2, 0));
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Kind", 20) == get_column(*cm2, 1));
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Price", 53) == get_column(*cm2, 2));
			}


			[TestMethod]
			void MissingColumnsAreNotUpdatedAndInvalidOrderColumnIsFixedOnModelStateLoad()
			{
				// INIT
				columns_model::column columns[] = {
					columns_model::column("col1", L"", 13, columns_model::dir_none),
					columns_model::column("col2", L"", 17, columns_model::dir_ascending),
					columns_model::column("col3", L"", 31, columns_model::dir_ascending),
				};
				shared_ptr<columns_model> cm(new columns_model(columns, 0, false));

				map<string, int> int_values;
				map<string, wstring> str_values;
				shared_ptr<mock_hive> h(new mock_hive(int_values, str_values));

				int_values["OrderBy"] = 3;
				int_values["OrderDirection"] = 0;
				str_values["col1/Caption"] = L"Contract";
				int_values["col1/Width"] = 233;
				str_values["col3/Caption"] = L"Price";

				// ACT
				cm->update(*h);

				// ASSERT
				Assert::IsTrue(make_pair(columns_model::npos, false) == cm->get_sort_order());
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Contract", 233) == get_column(*cm, 0));
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"", 17) == get_column(*cm, 1));
				Assert::IsTrue(wpl::ui::listview::columns_model::column(L"Price", 31) == get_column(*cm, 2));
			}
		};
	}
}
