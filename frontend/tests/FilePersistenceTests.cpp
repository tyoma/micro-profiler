#include <frontend/persistence.h>

#include "comparisons.h"
#include "helpers.h"
#include "legacy_serialization.h"
#include "mocks.h"
#include "primitive_helpers.h"

#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <test-helpers/helpers.h>
#include <test-helpers/primitive_helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace strmd
{
	template <> struct version<micro_profiler::tests::file_components> {
		enum {	value = version<micro_profiler::profiling_session>::value	};
	};
}

namespace micro_profiler
{
	inline bool operator <(const symbol_info &lhs, const symbol_info &rhs)
	{
		return make_pair(lhs.name, make_pair(lhs.rva, make_pair(lhs.size, make_pair(lhs.file_id, lhs.line))))
			< make_pair(rhs.name, make_pair(rhs.rva, make_pair(rhs.size, make_pair(rhs.file_id, rhs.line))));
	}

	inline bool operator <(const mapped_module_ex &lhs, const mapped_module_ex &rhs)
	{
		return make_pair(lhs.persistent_id, make_pair(lhs.path, lhs.base))
			< make_pair(rhs.persistent_id, make_pair(rhs.path, rhs.base));
	}

	inline bool operator <(const module_info_metadata &lhs, const module_info_metadata &rhs)
	{	return make_pair(lhs.path, lhs.symbols) < make_pair(rhs.path, rhs.symbols);	}

	inline bool operator <(const thread_info &lhs, const thread_info &rhs)
	{	return make_pair(lhs.native_id, lhs.description) < make_pair(rhs.native_id, rhs.description);	}

	inline bool operator ==(const initialization_data &lhs, const initialization_data &rhs)
	{	return make_pair(lhs.executable, lhs.ticks_per_second) == make_pair(rhs.executable, rhs.ticks_per_second);	}

	namespace tests
	{
		template <typename ArchiveT>
		void serialize(ArchiveT &a, file_components &d, unsigned /*ver*/)
		{
			a(d.process_info);
			a(d.mappings);
			a(d.modules);
			a(d.statistics);
			a(d.threads);
	//		a(d.patches);
		}

		namespace
		{
			profiling_session create_context()
			{
				profiling_session ctx = {
					{},
					make_shared<tables::statistics>(),
					make_shared<tables::module_mappings>(),
					make_shared<tables::modules>(),
					make_shared<tables::patches>(),
					make_shared<tables::threads>(),
				};
				return ctx;
			}

			template <typename ContainerT>
			void append(calls_statistics_table &statistics, const ContainerT &items, id_t parent_id = 0)
			{
				for (auto i = begin(items); i != end(items); ++i)
				{
					auto r = statistics.create();

					(*r).thread_id = i->first.second;
					(*r).parent_id = parent_id;
					(*r).address = i->first.first;
					static_cast<function_statistics &>(*r) = i->second;
					r.commit();
				}
			}
		}

		begin_test_suite( FilePersistenceTests )
			vector_adapter _buffer;
			strmd::serializer<vector_adapter, packer> ser;
			strmd::deserializer<vector_adapter, packer> dser;
			strmd::deserializer<vector_adapter, packer, 3> dser3;
			strmd::deserializer<vector_adapter, packer, 4> dser4;

			module_info_metadata modules[3];
			mapped_module_ex mappings[3];
			vector< pair<statistic_types::key, statistic_types::function_detailed> > statistics[4];
			vector< pair<statistic_types_t<long_address_t>::key, statistic_types_t<long_address_t>::function_detailed> > ustatistics[2];
			vector< pair<unsigned, thread_info> > threads[2];

			
			FilePersistenceTests()
				: ser(_buffer), dser(_buffer), dser3(_buffer), dser4(_buffer)
			{	}

			init( CreatePrerequisites )
			{
				symbol_info symbols1[] = {
					{	"free", 0x005, 10,	}, {	"malloc", 0x013, 11,	}, {	"realloc", 0x017, 12,	},
				};
				symbol_info symbols2[] = {
					{	"qsort", 0x105, 23,	}, {	"min", 0x013, 29,	}, {	"max", 0x170, 31,	}, {	"nth_element", 0x123, 37,	},
				};
				symbol_info symbols3[] = {
					{	"stable_sort", 0xFFF, 97,	},
				};
				mapped_module_ex mappings_[] = {
					{	10u, "c:\\windows\\kernel32.exe", 0x100000 },
					{	4u, "/usr/bin/TEST", 0xF00010 },
					{	2u, "c:\\Program File\\test\\test.exe", 0x9000000 },
				};

				modules[0].symbols = mkvector(symbols1);
				modules[1].symbols = mkvector(symbols2);
				modules[2].symbols = mkvector(symbols3);

				copy_n(mappings_, 3, mappings);

				statistics[0] = plural
					+ make_statistics(addr(0x100005), 123, 0, 1000, 0, 0, plural
						+ make_statistics(addr(0x1ull), 1023, 0, 1000, 0, 0))
					+ make_statistics(addr(0x100017, 3), 12, 0, 0, 0, 0)
					+ make_statistics(addr(0xF00115, 4), 127, 0, 0, 0, 0)
					+ make_statistics(addr(0xF00133, 3), 12000, 0, 250, 0, 0);
				statistics[1] = plural
					+ make_statistics(addr(0xF00115), 123, 0, 1000, 0, 0)
					+ make_statistics(addr(0xF00023), 12, 0, 9, 0, 0)
					+ make_statistics(addr(0xF00180), 127, 0, 10, 0, 0)
					+ make_statistics(addr(0xF00133), 127, 0, 8, 0, 0)
					+ make_statistics(addr(0x9000FFF), 12000, 0, 250, 0, 0)
					+ make_statistics(addr(0x9000FFF, 17), 12000, 0, 250, 0, 0);

				statistics[2] = plural
					+ make_statistics(addr(0x100005, 0), 123, 0, 1000, 0, 0)
					+ make_statistics(addr(0x100017, 0), 12, 0, 0, 0, 0)
					+ make_statistics(addr(0xF00115, 0), 127, 0, 0, 0, 0)
					+ make_statistics(addr(0xF00133, 0), 12000, 0, 250, 0, 0);
				statistics[3] = plural
					+ make_statistics(addr(0xF00115, 0), 123, 0, 1000, 0, 0)
					+ make_statistics(addr(0xF00023, 0), 12, 0, 9, 0, 0)
					+ make_statistics(addr(0xF00180, 0), 127, 0, 10, 0, 0)
					+ make_statistics(addr(0xF00133, 0), 127, 0, 8, 0, 0)
					+ make_statistics(addr(0x9000FFF, 0), 12000, 0, 250, 0, 0);

				ustatistics[0] = plural
					+ make_statistics(0x100005ull, 123, 0, 1000, 0, 0, plural
						+ make_statistics(0x1ull, 1023, 0, 1000, 0, 0))
					+ make_statistics(0x100017ull, 12, 0, 0, 0, 0)
					+ make_statistics(0xF00115ull, 127, 0, 0, 0, 0)
					+ make_statistics(0xF00133ull, 12000, 0, 250, 0, 0);
				ustatistics[1] = plural
					+ make_statistics(0xF00115ull, 123, 0, 1000, 0, 0)
					+ make_statistics(0xF00023ull, 12, 0, 9, 0, 0)
					+ make_statistics(0xF00180ull, 127, 0, 10, 0, 0)
					+ make_statistics(0xF00133ull, 127, 0, 8, 0, 0)
					+ make_statistics(0x9000FFFull, 12000, 0, 250, 0, 0);

				threads[0] = plural
					+ make_thread_info(1, 111, "#1")
					+ make_thread_info(3, 112, "#2")
					+ make_thread_info(4, 113, "#3");
				threads[1] = plural
					+ make_thread_info(1, 1211, "thread A")
					+ make_thread_info(17, 1212, "thread B");
			}


			test( ContentIsFullySerialized )
			{
				// INIT
				auto ctx = create_context();
				ctx.process_info.executable = "kjsdhgkjsdwwp.exe";
				ctx.process_info.ticks_per_second = 0xF00000000ull;
				append(*ctx.statistics, statistics[0]);
				assign(*ctx.module_mappings, plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]));
				assign(*ctx.modules, plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]));
				assign(*ctx.threads, threads[0]);

				// ACT
				ser(ctx);

				// ASSERT
				file_components components1;
				dser(components1);

				assert_equal(ctx.process_info, components1.process_info);
				assert_equivalent(*ctx.module_mappings, components1.mappings);
				assert_equivalent(*ctx.modules, components1.modules);
				assert_equivalent(*ctx.statistics, components1.statistics);

				thread_info reference1[] = {
					{	111, "#1",	}, {	112, "#2",	}, {	113, "#3",	},
				};

				assert_equivalent(plural
					+ make_pair(1u, reference1[0]) + make_pair(3u, reference1[1]) + make_pair(4u, reference1[2]),
					components1.threads);
				
				// INIT
				ctx = create_context();
				ctx.process_info.executable = "/usr/bin/grep";
				ctx.process_info.ticks_per_second = 0x1000ull;
				append(*ctx.statistics, statistics[1]);
				assign(*ctx.module_mappings, plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]));
				assign(*ctx.modules, plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]));
				assign(*ctx.threads, threads[1]);

				// ACT
				ser(ctx);

				// ASSERT
				file_components components2;
				dser(components2);

				assert_equal(ctx.process_info, components2.process_info);
				assert_equivalent(*ctx.module_mappings, components2.mappings);
				assert_equivalent(*ctx.modules, components2.modules);
				assert_equivalent(*ctx.statistics, components2.statistics);

				thread_info reference2[] = {
					{	1211, "thread A",	}, {	1212, "thread B",	},
				};

				assert_equivalent(plural
					+ make_pair(1u, reference2[0]) + make_pair(17u, reference2[1]),
					components2.threads);
			}


			test( ContentIsFullyDeserialized )
			{
				// INIT
				file_components components1;
				auto ctx1 = create_context();
				thread_info threads1[] = {
					{	111, "#1",	}, {	112, "#2",	}, {	113, "#3",	},
				};

				components1.process_info.executable = "kjsdhgkjsdwwp.exe";
				components1.process_info.ticks_per_second = 0xF00000000ull;
				append(components1.statistics, statistics[0]);
				assign_basic(components1.mappings, plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]));
				assign_basic(components1.modules, plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]));
				assign_basic(components1.threads, plural
					+ make_pair(1u, threads1[0]) + make_pair(3u, threads1[1]) + make_pair(4u, threads1[2]));
				ser(components1);

				// ACT
				dser(ctx1);

				// ASSERT
				assert_equal(components1.process_info, ctx1.process_info);
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0x100005, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 3, 0, 0x100017, 12, 0, 0, 0, 0)
					+ make_call_statistics(0, 4, 0, 0xF00115, 127, 0, 0, 0, 0)
					+ make_call_statistics(0, 3, 0, 0xF00133, 12000, 0, 250, 0, 0), *ctx1.statistics);
				assert_equivalent(plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx1.module_mappings);
				assert_equivalent(plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx1.modules);
				assert_equivalent(plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx1.modules);
				assert_equivalent(plural + make_pair(1u, threads1[0]) + make_pair(3u, threads1[1]) + make_pair(4u, threads1[2]), (containers::unordered_map<unsigned int, thread_info> &)*ctx1.threads);

				// INIT
				file_components components2;
				auto ctx2 = create_context();
				thread_info threads2[] = {
					{	1211, "thread A",	}, {	1212, "thread ABC",	},
				};

				components2.process_info.executable = "/usr/bin/grep";
				components2.process_info.ticks_per_second = 0x1000ull;
				append(components2.statistics, statistics[1]);
				assign_basic(components2.mappings, plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]));
				assign_basic(components2.modules, plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]));
				assign_basic(components2.threads, plural
					+ make_pair(1u, threads2[0]) + make_pair(17u, threads2[1]));
				ser(components2);

				// ACT
				dser(ctx2);

				// ASSERT
				assert_equal(components2.process_info, ctx2.process_info);
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0xF00115, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00023, 12, 0, 9, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00180, 127, 0, 10, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00133, 127, 0, 8, 0, 0)
					+ make_call_statistics(0, 1, 0, 0x9000FFF, 12000, 0, 250, 0, 0)
					+ make_call_statistics(0, 17, 0, 0x9000FFF, 12000, 0, 250, 0, 0), *ctx2.statistics);
				assert_equivalent(plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx2.module_mappings);
				assert_equivalent(plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx2.modules);
				assert_equivalent(plural + make_pair(1u, threads2[0]) + make_pair(17u, threads2[1]), (containers::unordered_map<unsigned int, thread_info> &)*ctx2.threads);
			}


			test( FileV3ContentIsFullyDeserialized )
			{
				// INIT
				file_v3_components components1;
				auto ctx1 = create_context();

				components1.ticks_per_second = 0xF00000000ull;
				assign_basic(components1.statistics, ustatistics[0]);
				assign_basic(components1.mappings, plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]));
				assign_basic(components1.modules, plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]));
				serialize_legacy(_buffer, components1);

				// ACT
				dser3(ctx1);

				// ASSERT
				assert_equal(0xF00000000ll, ctx1.process_info.ticks_per_second);
				assert_is_empty(ctx1.process_info.executable);
				assert_equivalent(plural
					+ make_call_statistics(0, 0, 0, 0x100005, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 0, 1, 0x1, 1023, 0, 1000, 0, 0)
					+ make_call_statistics(0, 0, 0, 0x100017, 12, 0, 0, 0, 0)
					+ make_call_statistics(0, 0, 0, 0xF00115, 127, 0, 0, 0, 0)
					+ make_call_statistics(0, 0, 0, 0xF00133, 12000, 0, 250, 0, 0), *ctx1.statistics);
				assert_equivalent(plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx1.module_mappings);
				assert_equivalent(plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx1.modules);
				assert_is_empty(*ctx1.threads);

				// INIT
				file_v3_components components2;
				auto ctx2 = create_context();

				components2.ticks_per_second = 0x1000ull;
				assign_basic(components2.statistics, ustatistics[1]);
				assign_basic(components2.mappings, plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]));
				assign_basic(components2.modules, plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]));
				serialize_legacy(_buffer, components2);

				// ACT
				dser3(ctx2);

				// ASSERT
				assert_equal(0x1000ll, ctx2.process_info.ticks_per_second);
				assert_is_empty(ctx2.process_info.executable);
				assert_equivalent(plural
					+ make_call_statistics(0, 0, 0, 0xF00115, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 0, 0, 0xF00023, 12, 0, 9, 0, 0)
					+ make_call_statistics(0, 0, 0, 0xF00180, 127, 0, 10, 0, 0)
					+ make_call_statistics(0, 0, 0, 0xF00133, 127, 0, 8, 0, 0)
					+ make_call_statistics(0, 0, 0, 0x9000FFF, 12000, 0, 250, 0, 0), *ctx2.statistics);
				assert_equivalent(plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx2.module_mappings);
				assert_equivalent(plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx2.modules);
				assert_is_empty(*ctx2.threads);
			}


			test( FileV4ContentIsFullyDeserialized )
			{
				// INIT
				file_v4_components components1;
				auto ctx1 = create_context();
				thread_info threads1[] = {
					{	111, "#1",	}, {	112, "#2",	}, {	113, "#3",	},
				};

				components1.ticks_per_second = 0xF00000000ull;
				assign_basic(components1.statistics, statistics[0]);
				assign_basic(components1.mappings, plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]));
				assign_basic(components1.modules, plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]));
				assign_basic(components1.threads, plural
					+ make_pair(1u, threads1[0]) + make_pair(3u, threads1[1]) + make_pair(4u, threads1[2]));
				serialize_legacy(_buffer, components1);

				// ACT
				dser4(ctx1);

				// ASSERT
				assert_equal(0xF00000000ll, ctx1.process_info.ticks_per_second);
				assert_is_empty(ctx1.process_info.executable);
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0x100005, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 1, 1, 0x1, 1023, 0, 1000, 0, 0)
					+ make_call_statistics(0, 3, 0, 0x100017, 12, 0, 0, 0, 0)
					+ make_call_statistics(0, 4, 0, 0xF00115, 127, 0, 0, 0, 0)
					+ make_call_statistics(0, 3, 0, 0xF00133, 12000, 0, 250, 0, 0), *ctx1.statistics);
				assert_equivalent(plural + make_pair(10u, mappings[0]) + make_pair(11u, mappings[1]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx1.module_mappings);
				assert_equivalent(plural + make_pair(10u, modules[0]) + make_pair(4u, modules[1]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx1.modules);
				assert_equivalent(plural + make_pair(1u, threads1[0]) + make_pair(3u, threads1[1]) + make_pair(4u, threads1[2]), (containers::unordered_map<unsigned int, thread_info> &)*ctx1.threads);

				// INIT
				file_v4_components components2;
				auto ctx2 = create_context();
				thread_info threads2[] = {
					{	1211, "thread A",	}, {	1212, "thread ABC",	},
				};

				components2.ticks_per_second = 0x1000ull;
				assign_basic(components2.statistics, statistics[1]);
				assign_basic(components2.mappings, plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]));
				assign_basic(components2.modules, plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]));
				assign_basic(components2.threads, plural
					+ make_pair(1u, threads2[0]) + make_pair(17u, threads2[1]));
				serialize_legacy(_buffer, components2);

				// ACT
				dser4(ctx2);

				// ASSERT
				assert_equal(0x1000ll, ctx2.process_info.ticks_per_second);
				assert_is_empty(ctx2.process_info.executable);
				assert_equivalent(plural
					+ make_call_statistics(0, 1, 0, 0xF00115, 123, 0, 1000, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00023, 12, 0, 9, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00180, 127, 0, 10, 0, 0)
					+ make_call_statistics(0, 1, 0, 0xF00133, 127, 0, 8, 0, 0)
					+ make_call_statistics(0, 1, 0, 0x9000FFF, 12000, 0, 250, 0, 0)
					+ make_call_statistics(0, 17, 0, 0x9000FFF, 12000, 0, 250, 0, 0), *ctx2.statistics);
				assert_equivalent(plural + make_pair(0u, mappings[1]) + make_pair(1u, mappings[2]), (containers::unordered_map<unsigned int, mapped_module_ex> &)*ctx2.module_mappings);
				assert_equivalent(plural + make_pair(4u, modules[1]) + make_pair(2u, modules[2]), (containers::unordered_map<unsigned int, module_info_metadata> &)*ctx2.modules);
				assert_equivalent(plural + make_pair(1u, threads2[0]) + make_pair(17u, threads2[1]), (containers::unordered_map<unsigned int, thread_info> &)*ctx2.threads);
			}

		end_test_suite
	}
}
