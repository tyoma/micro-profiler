#include <collector/statistics_bridge.h>

#include <collector/module_tracker.h>

#include "helpers.h"
#include "mocks.h"

#include <algorithm>
#include <common/module.h>
#include <common/path.h>
#include <common/time.h>
#include <test-helpers/comparisons.h>
#include <test-helpers/constants.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			const overhead c_overhead(17, 0);

			typedef pair<unsigned, statistic_types_t<unsigned>::function_detailed> addressed_statistics;

			template <typename ContainerT>
			const symbol_info *get_symbol_by_name(const ContainerT &symbols, const char *name)
			{
				for (typename ContainerT::const_iterator i = symbols.begin(); i != symbols.end(); ++i)
					if (string::npos != i->name.find(name))
						return &*i;
				assert_is_true(false);
				return 0;
			}

			template <typename ContainerT>
			string get_file(const ContainerT &files, unsigned id)
			{
				for (typename ContainerT::const_iterator i = files.begin(); i != files.end(); ++i)
					if (id == i->first)
						return i->second;
				assert_is_true(false);
				return 0;
			}
		}

		begin_test_suite( StatisticsBridgeTests )
			shared_ptr<mocks::tracer> cc;
			shared_ptr<mocks::frontend_state> state;
			shared_ptr<ipc::channel> frontend;
			vector<mocks::thread_statistics_map> update_log;
			shared_ptr<module_tracker> mtracker;
			shared_ptr<mocks::thread_monitor> tmonitor;
			unsigned int ref_count;

			init( CreateQueue )
			{
				mtracker.reset(new module_tracker);
				tmonitor.reset(new mocks::thread_monitor);
			}


			init( InitializeFrontendMock )
			{
				ref_count = 0;
				cc.reset(new mocks::tracer);
				state.reset(new mocks::frontend_state(cc));
				state->updated = [this] (const mocks::thread_statistics_map &u) { update_log.push_back(u); };
				frontend = state->create();
			}


			test( FrontendIsInitializedAtBridgeConstruction )
			{
				// INIT
				initialization_data process_init;

				state->initialized = [&] (const initialization_data &id) { process_init = id; };

				// ACT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);

				// ASSERT
				assert_equal(get_current_executable(), process_init.executable);
				assert_approx_equal(ticks_per_second(), process_init.ticks_per_second, 0.01);
			}


			test( EmptyUpdateIsSentIfNoAnalysisHasBeenDoneOrAnalysisResultIsEmpty )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);

				// ACT
				b.update_frontend();

				// ASSERT
				assert_equal(1u, update_log.size());
				assert_is_empty(update_log[0]);

				// ACT
				b.analyze();
				b.update_frontend();

				// ASSERT
				assert_equal(2u, update_log.size());
				assert_is_empty(update_log[1]);
			}


			test( FrontendUpdateClearsTheAnalyzer )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);
				call_record trace[] = {
					{	0, addr(0x1223)	},
					{	10 + c_overhead.inner, addr(0)	},
				};

				cc->on_read_collected = [&] (calls_collector_i::acceptor &a) {	
					a.accept_calls(141, trace, 2);
				};

				b.analyze();
				b.update_frontend();

				// ACT
				b.update_frontend();

				// ASSERT
				assert_equal(2u, update_log.size());
				assert_equal(1u, update_log[1].size());
				assert_equal(141u, update_log[1].begin()->first);
				assert_is_empty(update_log[1].begin()->second);
			}


			test( CollectedCallsArePassedToFrontend )
			{
				// INIT
				vector<mocks::thread_statistics_map> update_log1, update_log2;
				mocks::tracer cc1, cc2;
				overhead o1(23, 0), o2(31, 0);
				shared_ptr<mocks::frontend_state> state1(state),
					state2(new mocks::frontend_state(shared_ptr<void>()));
				shared_ptr<ipc::channel> frontend2 = state2->create();
				statistics_bridge b1(cc1, o1, *frontend, mtracker, tmonitor),
					b2(cc2, o2, *frontend2, mtracker, tmonitor);
				call_record trace1[] = {
					{	0, addr(0x1223)	},
					{	10 + o1.inner, addr(0)	},
					{	1000, addr(0x1223)	},
					{	1029 + o1.inner, addr(0)	},
				};
				call_record trace2[] = {
					{	0, addr(0x2223)	},
					{	13 + o2.inner, addr(0)	},
					{	1000, addr(0x3223)	},
					{	1017 + o2.inner, addr(0)	},
					{	2000, addr(0x4223)	},
					{	2019 + o2.inner, addr(0)	},
				};
				state1->updated = [&] (const mocks::thread_statistics_map &u) { update_log1.push_back(u); };
				state2->updated = [&] (const mocks::thread_statistics_map &u) { update_log2.push_back(u); };

				cc1.on_read_collected = [&] (calls_collector_i::acceptor &a) {	a.accept_calls(18, trace1, 4);	};
				cc2.on_read_collected = [&] (calls_collector_i::acceptor &a) {	a.accept_calls(18881, trace2, 6);	};

				// ACT
				b1.analyze();
				b1.update_frontend();
				b2.analyze();
				b2.update_frontend();

				// ASSERT
				addressed_statistics reference1[] = {
					make_statistics(0x1223u, 2, 0, 39, 39, 29),
				};
				addressed_statistics reference2[] = {
					make_statistics(0x2223u, 1, 0, 13, 13, 13),
					make_statistics(0x3223u, 1, 0, 17, 17, 17),
					make_statistics(0x4223u, 1, 0, 19, 19, 19),
				};

				assert_equal(1u, update_log1.size());
				assert_equal(1u, update_log1[0].size());
				assert_equal(18u, update_log1[0].begin()->first);
				assert_equivalent(reference1, update_log1[0].begin()->second);

				assert_equal(1u, update_log2.size());
				assert_equal(1u, update_log2[0].size());
				assert_equal(18881u, update_log2[0].begin()->first);
				assert_equivalent(reference2, update_log2[0].begin()->second);
			}


			test( LoadedModulesAreReportedOnUpdate )
			{
				// INIT
				vector<loaded_modules> loads;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);

				state->modules_loaded = [&] (const loaded_modules &m) { loads.push_back(m); };

				// ACT
				image image0(c_symbol_container_1);
				b.update_frontend();

				// ASSERT
				assert_equal(1u, loads.size());
				assert_not_null(find_module(loads[0], c_symbol_container_1));

				// ACT
				image image1(c_symbol_container_2);
				image image3(c_symbol_container_3_nosymbols);
				b.update_frontend();

				// ASSERT
				assert_equal(2u, loads.size());
				assert_equal(2u, loads[1].size());
				assert_not_null(find_module(loads[1], c_symbol_container_2));
				assert_not_null(find_module(loads[1], c_symbol_container_3_nosymbols));
			}


			test( UnloadedModulesAreReportedOnUpdate )
			{
				// INIT
				loaded_modules l;
				vector<unloaded_modules> unloads;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);

				state->modules_loaded = [&] (const loaded_modules &m) { l = m; };
				state->modules_unloaded = [&] (const unloaded_modules &m) { unloads.push_back(m); };

				auto_ptr<image> image0(new image(c_symbol_container_1));
				auto_ptr<image> image1(new image(c_symbol_container_2));
				auto_ptr<image> image2(new image(c_symbol_container_3_nosymbols));
				b.update_frontend();

				const mapped_module_identified *mmi[] = {
					find_module(l, c_symbol_container_1), find_module(l, c_symbol_container_2), find_module(l, c_symbol_container_3_nosymbols),
				};

				// ACT
				image1.reset();
				b.update_frontend();

				// ASSERT
				assert_equal(1u, unloads.size());

				unsigned reference1[] = { mmi[1]->instance_id, };

				assert_equivalent(reference1, unloads[0]);

				// ACT
				image0.reset();
				image2.reset();
				b.update_frontend();

				// ASSERT
				assert_equal(2u, unloads.size());

				unsigned reference2[] = { mmi[0]->instance_id, mmi[2]->instance_id, };

				assert_equivalent(reference2, unloads[1]);
			}


			test( ModuleMetadataIsSerializedOnRequest )
			{
				// INIT
				unsigned persistent_id;
				module_info_metadata md;
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);
				loaded_modules l;
				unloaded_modules u;

				state->metadata_received = [&] (unsigned id, const module_info_metadata &md_) {
					persistent_id = id, md = md_;
				};

				image image0(c_symbol_container_1);
				image image1(c_symbol_container_2);

				mtracker->get_changes(l, u);

				const mapped_module_identified *mmi[] = { find_module(l, c_symbol_container_1), find_module(l, c_symbol_container_2), };

				// ACT
				b.send_module_metadata(mmi[1]->persistent_id);

				// ASSERT
				assert_equal(persistent_id, mmi[1]->persistent_id);

				const symbol_info *symbol = get_symbol_by_name(md.symbols, "get_function_addresses_2");

				assert_not_null(symbol);
				assert_equal(image1.get_symbol_rva("get_function_addresses_2"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				symbol = get_symbol_by_name(md.symbols, "guinea_snprintf");

				assert_not_null(symbol);
				assert_equal(image1.get_symbol_rva("guinea_snprintf"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_2.cpp", *get_file(md.source_files, symbol->file_id));
#endif

				// ACT
				b.send_module_metadata(mmi[0]->persistent_id);

				// ASSERT
				assert_equal(persistent_id, mmi[0]->persistent_id);

				symbol = get_symbol_by_name(md.symbols, "get_function_addresses_1");

				assert_not_null(symbol);
				assert_equal(image0.get_symbol_rva("get_function_addresses_1"), symbol->rva);
#ifdef _WIN32
				assert_equal("symbol_container_1.cpp", *get_file(md.source_files, symbol->file_id));
#endif
			}


			test( ThreadInfoIsSerializedOnRequest )
			{
				// INIT
				statistics_bridge b(*cc, c_overhead, *frontend, mtracker, tmonitor);
				thread_info ti[] = {
					{ 1221, "thread 1", mt::milliseconds(190212), mt::milliseconds(0), mt::milliseconds(1902), false },
					{ 171717, "thread 2", mt::milliseconds(112), mt::milliseconds(0), mt::milliseconds(2900), false },
					{ 17171, "thread #3", mt::milliseconds(112), mt::milliseconds(3000), mt::milliseconds(900), true },
				};
				thread_monitor::thread_id request1[] = { 1, 19, }, request2[] = { 1, 19, 2, };
				vector< pair<unsigned /*thread_id*/, thread_info> > threads;

				state->threads_received = [&] (const vector< pair<thread_monitor::thread_id, thread_info> > &threads_) {
					threads = threads_;
				};

				tmonitor->add_info(1 /*thread_id*/, ti[0]);
				tmonitor->add_info(2 /*thread_id*/, ti[1]);
				tmonitor->add_info(19 /*thread_id*/, ti[2]);

				// ACT
				b.send_thread_info(mkvector(request1));

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference1[] = {
					make_pair(1, ti[0]), make_pair(19, ti[2]),
				};

				assert_equal(reference1, threads);

				// ACT
				b.send_thread_info(mkvector(request2));

				// ASSERT
				pair<thread_monitor::thread_id, thread_info> reference2[] = {
					make_pair(1, ti[0]), make_pair(19, ti[2]), make_pair(2, ti[1]),
				};

				assert_equal(reference2, threads);
			}
		end_test_suite
	}
}
