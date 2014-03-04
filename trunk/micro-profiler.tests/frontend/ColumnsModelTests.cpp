#include <frontend/columns_model.h>

#include "../assert.h"

#include <common/configuration.h>

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

			class mock_hive : public enable_shared_from_this<mock_hive>, public hive
			{
			public:
				enum entry_type { hive_create, int_store, str_store };
				struct entry;

			public:
				mock_hive(int &id, vector<entry> &log);

			private:
				const mock_hive &operator =(const mock_hive &rhs);

				virtual void store(const char *name, int value);
				virtual void store(const char *name, const wchar_t *value);
				virtual shared_ptr<hive> create(const char *name);

			private:
				int &_id;
				vector<entry> &_log;
				const int _myid;
			};

			struct mock_hive::entry
			{
				int id;
				entry_type type;

				string name;

				int int_value; // or parent node id, when type == 'hive_create'
				wstring str_value;
			};

			bool operator <(const mock_hive::entry &lhs, const mock_hive::entry &rhs)
			{
				if (lhs.id != rhs.id)
					return lhs.id < rhs.id;
				else if (lhs.type != rhs.type)
					return lhs.type < rhs.type;
				else
					switch (lhs.type)
					{
					case mock_hive::hive_create:
						return make_pair(lhs.name, lhs.int_value) < make_pair(rhs.name, rhs.int_value);

					case mock_hive::int_store:
						return make_pair(lhs.name, lhs.int_value) < make_pair(rhs.name, rhs.int_value);

					case mock_hive::str_store:
						return make_pair(lhs.name, lhs.str_value) < make_pair(rhs.name, rhs.str_value);

					default:
						return false;
					}
			}


			mock_hive::mock_hive(int &id, vector<entry> &log)
				: _id(id), _log(log), _myid(id++)
			{	}

			void mock_hive::store(const char *name, int value)
			{
				entry e = { _myid, int_store, name, value };
				_log.push_back(e);
			}

			void mock_hive::store(const char *name, const wchar_t *value)
			{
				entry e = { _myid, str_store, name, 0, value };
				_log.push_back(e);
			}

			shared_ptr<hive> mock_hive::create(const char *name)
			{
				entry e = { _id, hive_create, name, _myid };
				_log.push_back(e);
				return shared_ptr<hive>(new mock_hive(_id, _log));
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
					columns_model::column("id1", L"", columns_model::dir_none),
					columns_model::column("id2", L"", columns_model::dir_descending),
					columns_model::column("id3", L"", columns_model::dir_descending),
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
					columns_model::column("id1", L"", columns_model::dir_none),
					columns_model::column("id2", L"", columns_model::dir_descending),
					columns_model::column("id3", L"", columns_model::dir_ascending),
					columns_model::column("id4", L"", columns_model::dir_descending),
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
					columns_model::column("id1", L"", columns_model::dir_none),
					columns_model::column("id2", L"", columns_model::dir_descending),
					columns_model::column("id3", L"", columns_model::dir_ascending),
					columns_model::column("id4", L"", columns_model::dir_descending),
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
					columns_model::column("id1", L"first", columns_model::dir_none),
					columns_model::column("id2", L"second", columns_model::dir_descending),
					columns_model::column("id3", L"third", columns_model::dir_ascending),
					columns_model::column("id4", L"fourth", columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", columns_model::dir_none),
					columns_model::column("id2", L"a second column", columns_model::dir_ascending),
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
					columns_model::column("id1", L"first", columns_model::dir_none),
					columns_model::column("id2", L"second", columns_model::dir_descending),
					columns_model::column("id3", L"third", columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", columns_model::dir_none),
					columns_model::column("id2", L"a second column", columns_model::dir_ascending),
				};
				shared_ptr<listview::columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<listview::columns_model> cm2(new columns_model(columns2, 0, false));
				listview::columns_model::column c;

				// ACT
				cm1->get_column(0, c);

				// ASSERT
				Assert::IsTrue(L"first" == c);

				// ACT
				cm1->get_column(2, c);

				// ASSERT
				Assert::IsTrue(L"third" == c);

				// ACT
				cm2->get_column(0, c);

				// ASSERT
				Assert::IsTrue(L"a first column" == c);

				// ACT
				cm2->get_column(1, c);

				// ASSERT
				Assert::IsTrue(L"a second column" == c);
			}


			[TestMethod]
			void UnchangedModelIsStoredAsProvidedAtConstruction()
			{
				// INIT
				columns_model::column columns1[] = {
					columns_model::column("id1", L"Index", columns_model::dir_none),
					columns_model::column("id2", L"Function", columns_model::dir_descending),
					columns_model::column("id3", L"Exclusive Time", columns_model::dir_ascending),
					columns_model::column("fourth", L"Inclusive Time", columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column("id1", L"a first column", columns_model::dir_none),
					columns_model::column("id2", L"a second column", columns_model::dir_ascending),
				};
				shared_ptr<columns_model> cm1(new columns_model(columns1, 0, false));
				shared_ptr<columns_model> cm2(new columns_model(columns2, listview::columns_model::npos, false));
				shared_ptr<columns_model> cm3(new columns_model(columns2, 1, true));

				int id = 0;
				vector<mock_hive::entry> log;
				shared_ptr<mock_hive> h(new mock_hive(id, log));

				// ACT
				cm1->store(*h);

				// ASSERT
				mock_hive::entry r1[] = {
					{ 0, mock_hive::int_store, "OrderBy", 0 },
					{ 0, mock_hive::int_store, "OrderDirection", 0 },
					{ 1, mock_hive::hive_create, "id1", 0 },
					{ 1, mock_hive::str_store, "Caption", 0, L"Index" },
					{ 2, mock_hive::hive_create, "id2", 0 },
					{ 2, mock_hive::str_store, "Caption", 0, L"Function" },
					{ 3, mock_hive::hive_create, "id3", 0 },
					{ 3, mock_hive::str_store, "Caption", 0, L"Exclusive Time" },
					{ 4, mock_hive::hive_create, "fourth", 0 },
					{ 4, mock_hive::str_store, "Caption", 0, L"Inclusive Time" },
				};

				assert::sequences_equal(r1, log);

				// INIT
				id = 1;
				log.clear();

				// ACT
				cm2->store(*h);
				cm3->store(*h);

				// ASSERT
				mock_hive::entry r2[] = {
					{ 0, mock_hive::int_store, "OrderBy", -1 },
					{ 0, mock_hive::int_store, "OrderDirection", 0 },
					{ 1, mock_hive::hive_create, "id1", 0 },
					{ 1, mock_hive::str_store, "Caption", 0, L"a first column" },
					{ 2, mock_hive::hive_create, "id2", 0 },
					{ 2, mock_hive::str_store, "Caption", 0, L"a second column" },
					{ 0, mock_hive::int_store, "OrderBy", 1 },
					{ 0, mock_hive::int_store, "OrderDirection", 1 },
					{ 3, mock_hive::hive_create, "id1", 0 },
					{ 3, mock_hive::str_store, "Caption", 0, L"a first column" },
					{ 4, mock_hive::hive_create, "id2", 0 },
					{ 4, mock_hive::str_store, "Caption", 0, L"a second column" },
				};

				assert::sequences_equal(r2, log);
			}
		};
	}
}
