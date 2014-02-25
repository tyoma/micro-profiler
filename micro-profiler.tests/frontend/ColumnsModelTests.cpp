#include <frontend/columns_model.h>

namespace std
{
	using tr1::shared_ptr;
	using tr1::bind;
	using namespace placeholders;
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
					columns_model::column(L"", columns_model::dir_none),
					columns_model::column(L"", columns_model::dir_descending),
					columns_model::column(L"", columns_model::dir_descending),
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
					columns_model::column(L"", columns_model::dir_none),
					columns_model::column(L"", columns_model::dir_descending),
					columns_model::column(L"", columns_model::dir_ascending),
					columns_model::column(L"", columns_model::dir_descending),
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
					columns_model::column(L"", columns_model::dir_none),
					columns_model::column(L"", columns_model::dir_descending),
					columns_model::column(L"", columns_model::dir_ascending),
					columns_model::column(L"", columns_model::dir_descending),
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
					columns_model::column(L"first", columns_model::dir_none),
					columns_model::column(L"second", columns_model::dir_descending),
					columns_model::column(L"third", columns_model::dir_ascending),
					columns_model::column(L"fourth", columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column(L"a first column", columns_model::dir_none),
					columns_model::column(L"a second column", columns_model::dir_ascending),
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
					columns_model::column(L"first", columns_model::dir_none),
					columns_model::column(L"second", columns_model::dir_descending),
					columns_model::column(L"third", columns_model::dir_ascending),
				};
				columns_model::column columns2[] = {
					columns_model::column(L"a first column", columns_model::dir_none),
					columns_model::column(L"a second column", columns_model::dir_ascending),
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
		};
	}
}
