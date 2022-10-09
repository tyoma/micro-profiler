#include <frontend/symbol_resolver.h>

#include "helpers.h"
#include "helpers_metadata.h"

#include <common/protocol.h>
#include <frontend/keyer.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			template <typename TableT, typename ContainerT>
			void add_records_invalidate(TableT &table_, const ContainerT &records)
			{
				add_records(table_, records);
				table_.invalidate();
			}
		}

		begin_test_suite( SymbolResolverTests )
			shared_ptr<tables::modules> modules;
			shared_ptr<tables::module_mappings> mappings;
			vector<unsigned> _requested;

			init( Init )
			{
				modules = make_shared<tables::modules>();
				mappings = make_shared<tables::module_mappings>();
				modules->request_presence = [this] (shared_ptr<void> &, unsigned persistent_id, tables::modules::metadata_ready_cb cb) {
					if (const auto i = sdb::unique_index<keyer::external_id>(*this->modules).find(persistent_id))
						cb(*i);
					_requested.push_back(persistent_id);
				};
			}


			test( EmptyNameIsReturnedWhenNoMetadataIsLoaded )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x00F));
				assert_is_empty(r->symbol_name_by_va(0x100));
			}


			test( EmptyNameIsReturnedForFunctionsBeforeTheirBegining )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols);

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x00F));
				assert_is_empty(r->symbol_name_by_va(0x100));
			}


			test( EmptyNameIsReturnedForFunctionsPassTheEnd )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols);

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x013));
				assert_is_empty(r->symbol_name_by_va(0x107));
			}


			test( FunctionsLoadedThroughAsMetadataAreSearchableByAbsoluteAddress )
			{
				// INIT
				symbol_info symbols[] = { { "foo", 0x1010, 3 }, { "bar_2", 0x1101, 5 }, };

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols);

				// ACT
				symbol_resolver r(modules, mappings);

				// ASSERT
				assert_equal("foo", r.symbol_name_by_va(0x1010));
				assert_equal("foo", r.symbol_name_by_va(0x1012));
				assert_equal("bar_2", r.symbol_name_by_va(0x1101));
				assert_equal("bar_2", r.symbol_name_by_va(0x1105));
			}


			test( SymbolsAreAvailableDuringInvalidation )
			{
				// INIT
				symbol_info symbols[] = { { "foo", 0x1010, 3 }, { "bar_2", 0x1101, 5 }, };
				auto cb = make_shared<tables::modules::metadata_ready_cb>();

				modules->request_presence = [&cb] (shared_ptr<void> &req, unsigned, tables::modules::metadata_ready_cb cb_) {
					cb = make_shared<tables::modules::metadata_ready_cb>(cb_);
					req = cb;
				};

				symbol_resolver r(modules, mappings);
				auto invalidations = 0;
				auto conn = r.invalidate += [&] {
					assert_equal("foo", r.symbol_name_by_va(0x1010));
					assert_equal("bar_2", r.symbol_name_by_va(0x1101));
					invalidations++;
				};

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));

				r.symbol_name_by_va(0x1010);

				// ACT
				(*cb)(create_metadata(symbols));

				// ASSERT
				assert_equal(1, invalidations);
			}


			test( FunctionsLoadedThroughAsMetadataAreOffsetAccordingly )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };
				
				add_metadata(*modules, 1, symbols);

				// ACT
				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0x1100000));

				// ASSERT
				assert_equal("foo", r->symbol_name_by_va(0x1100010));
				assert_equal("bar", r->symbol_name_by_va(0x1100101));

				// ACT
				add_records_invalidate(*mappings, plural + make_mapping(1, 1u, 0x1100501));

				// ASSERT
				assert_equal("foo", r->symbol_name_by_va(0x1100010));
				assert_equal("bar", r->symbol_name_by_va(0x1100101));

				assert_equal("foo", r->symbol_name_by_va(0x1100511));
				assert_equal("bar", r->symbol_name_by_va(0x1100602));
			}


			test( ConstantReferenceFromResolverIsTheSame )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, { "baz", 0x108, 5 }, };

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols);

				// ACT
				const string *name1_1 = &r->symbol_name_by_va(0x0010);
				const string *name2_1 = &r->symbol_name_by_va(0x0101);
				const string *name3_1 = &r->symbol_name_by_va(0x0108);

				const string *name2_2 = &r->symbol_name_by_va(0x101);
				const string *name1_2 = &r->symbol_name_by_va(0x010);
				const string *name3_2 = &r->symbol_name_by_va(0x108);

				// ASSERT
				assert_equal(name1_1, name1_2);
				assert_equal(name2_1, name2_2);
				assert_equal(name3_1, name3_2);
			}


			test( NoFileLineInformationIsReturnedForInvalidAddress )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_resolver::fileline_t dummy;

				// ACT / ASSERT
				assert_is_false(r->symbol_fileline_by_va(0, dummy));
			}


			test( FileLineInformationIsReturnedBySymolProvider )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 11, 121 },
					{ "b", 0x101, 5, 1, 1 },
					{ "c", 0x158, 5, 7, 71 },
					{ "d", 0x188, 5, 7, 713 },
				};
				pair<unsigned, string> files[] = {
					make_pair(11, "zoo.cpp"), make_pair(7, "c:/umi.cpp"), make_pair(1, "d:\\dev\\kloo.cpp"),
				};
				symbol_resolver::fileline_t results[4];

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols, files);

				// ACT / ASSERT
				assert_is_true(r->symbol_fileline_by_va(0x010, results[0]));
				assert_is_true(r->symbol_fileline_by_va(0x101, results[1]));
				assert_is_true(r->symbol_fileline_by_va(0x158, results[2]));
				assert_is_true(r->symbol_fileline_by_va(0x188, results[3]));

				// ASSERT
				symbol_resolver::fileline_t reference[] = {
					make_pair("zoo.cpp", 121),
					make_pair("d:\\dev\\kloo.cpp", 1),
					make_pair("c:/umi.cpp", 71),
					make_pair("c:/umi.cpp", 713),
				};

				assert_equal(reference, results);
			}


			test( FileLineInformationIsRebasedForNewMetadata )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
				symbol_info symbols1[] = {
					{ "a", 0x010, 3, 11, 121 },
					{ "b", 0x101, 5, 1, 1 },
					{ "c", 0x158, 5, 7, 71 },
					{ "d", 0x188, 5, 7, 713 },
				};
				symbol_info symbols2[] = {
					{ "a", 0x010, 3, 1, 121 },
					{ "b", 0x101, 5, 1, 1 },
					{ "c", 0x158, 5, 7, 71 },
					{ "d", 0x180, 5, 50, 71 },
				};
				pair<unsigned, string> files1[] = {
					make_pair(11, "zoo.cpp"), make_pair(1, "c:/umi.cpp"), make_pair(7, "d:\\dev\\kloo.cpp"),
				};
				pair<unsigned, string> files2[] = {
					make_pair(1, "ZOO.cpp"), make_pair(7, "/projects/umi.cpp"), make_pair(50, "d:\\dev\\abc.cpp"),
				};
				symbol_resolver::fileline_t results[8];

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols1, files1);

				// ACT / ASSERT
				add_records_invalidate(*mappings, plural + make_mapping(1, 2u, 0x8000));
				add_metadata(*modules, 2u, symbols2, files2);

				r->symbol_fileline_by_va(0x010, results[0]);
				r->symbol_fileline_by_va(0x101, results[1]);
				r->symbol_fileline_by_va(0x158, results[2]);
				r->symbol_fileline_by_va(0x188, results[3]);
				r->symbol_fileline_by_va(0x8010, results[4]);
				r->symbol_fileline_by_va(0x8101, results[5]);
				r->symbol_fileline_by_va(0x8158, results[6]);
				r->symbol_fileline_by_va(0x8180, results[7]);

				// ASSERT
				symbol_resolver::fileline_t reference[] = {
					make_pair("zoo.cpp", 121),
					make_pair("c:/umi.cpp", 1),
					make_pair("d:\\dev\\kloo.cpp", 71),
					make_pair("d:\\dev\\kloo.cpp", 713),
					make_pair("ZOO.cpp", 121),
					make_pair("ZOO.cpp", 1),
					make_pair("/projects/umi.cpp", 71),
					make_pair("d:\\dev\\abc.cpp", 71),
				};

				assert_equal(reference, results);
			}


			//test( SymbolResolverIsSerializable )
			//{
			//	// INIT
			//	vector_adapter buffer;
			//	strmd::serializer<vector_adapter, packer> ser(buffer);
			//	strmd::deserializer<vector_adapter, packer> dser(buffer);
			//	shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));
			//	module::mapping_instance basic = { };
			//	symbol_info symbols[] = {
			//		{ "a", 0x010, 3, 11, 121 },
			//		{ "b", 0x101, 5, 1, 1 },
			//		{ "cccc", 0x158, 5, 7, 71 },
			//		{ "ddd", 0x188, 5, 7, 713 },
			//	};
			//	pair<unsigned, string> files[] = {
			//		make_pair(11, "zoo.cpp"), make_pair(1, "c:/umi.cpp"), make_pair(7, "d:\\dev\\kloo.cpp"),
			//	};
			//	module_info_metadata metadata = { mkvector(symbols), mkvector(files) };
			//	symbol_resolver::fileline_t results[4];

			//	add_mapping(basic);
			//	add_metadata(*modules, basic.persistent_id, metadata);

			//	// ACT
			//	shared_ptr<symbol_resolver> r2(new symbol_resolver(modules, mappings));

			//	ser(*r);
			//	dser(*r2);

			//	// ASSERT
			//	assert_equal("a", r2->symbol_name_by_va(0x010));
			//	assert_equal("b", r2->symbol_name_by_va(0x101));
			//	assert_equal("cccc", r2->symbol_name_by_va(0x158));
			//	assert_equal("ddd", r2->symbol_name_by_va(0x188));

			//	assert_is_true(r2->symbol_fileline_by_va(0x010, results[0]));
			//	assert_is_true(r2->symbol_fileline_by_va(0x101, results[1]));
			//	assert_is_true(r2->symbol_fileline_by_va(0x158, results[2]));
			//	assert_is_true(r2->symbol_fileline_by_va(0x188, results[3]));

			//	symbol_resolver::fileline_t reference[] = {
			//		make_pair("zoo.cpp", 121),
			//		make_pair("c:/umi.cpp", 1),
			//		make_pair("d:\\dev\\kloo.cpp", 71),
			//		make_pair("d:\\dev\\kloo.cpp", 713),
			//	};

			//	assert_equal(reference, results);
			//}


			test( FilelineIsNotAvailableForMissingFunctions )
			{
				// INIT
				symbol_resolver r(modules, mappings);
				symbol_resolver::fileline_t result;

				// ACT / ASSERT
				assert_is_false(r.symbol_fileline_by_va(123, result));
			}


			test( FilelineIsNotAvailableForFunctionsWithMissingFiles )
			{
				// INIT
				symbol_resolver r(modules, mappings);
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 11, 121 },
					{ "b", 0x101, 5, 1, 1 },
					{ "cccc", 0x158, 5, 0, 71 },
					{ "ddd", 0x188, 5, 0, 713 },
				};
				symbol_resolver::fileline_t result;

				add_records_invalidate(*mappings, plural + make_mapping(0, 1u, 0));
				add_metadata(*modules, 1u, symbols);

				// ACT / ASSERT
				assert_is_false(r.symbol_fileline_by_va(0x010, result));
				assert_is_false(r.symbol_fileline_by_va(0x101, result));
				assert_is_false(r.symbol_fileline_by_va(0x158, result));
				assert_is_false(r.symbol_fileline_by_va(0x188, result));
			}


			test( SymbolResolverRequestsMetadataOnFindSymbolHits )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver(modules, mappings));

				add_records_invalidate(*mappings, plural
					+ make_mapping(0, 11, 0x120050, "c:\\dev\\test.exe", 1919191)
					+ make_mapping(1, 12, 0x120100, "nonono", 17)
					+ make_mapping(2, 11711, 0x200000, "agge.tests.dll", 117000001)
					+ make_mapping(3, 100, 0x310000, "/bin/bash", 100));

				// ACT
				r->symbol_name_by_va(0x120060);

				// ASSERT
				assert_equivalent(plural + 11u, _requested);

				// ACT
				r->symbol_name_by_va(0x311000);
				r->symbol_name_by_va(0x210000);

				// ASSERT
				assert_equivalent(plural + 11u + 100u + 11711u, _requested);
			}


			test( NewRequestHandlesArePassedForModuleRequests )
			{
				// INIT
				auto r = make_shared<symbol_resolver>(modules, mappings);
				vector< weak_ptr<void> > requests;

				modules->request_presence = [&] (shared_ptr<void> &req, unsigned, tables::modules::metadata_ready_cb) {
					req = make_shared<bool>();
					requests.push_back(req);
				};

				add_records_invalidate(*mappings, plural
					+ make_mapping(0, 11, 0x120050, "c:\\dev\\test.exe", 1919191)
					+ make_mapping(1, 12, 0x120100, "nonono", 17)
					+ make_mapping(2, 11711, 0x200000, "agge.tests.dll", 117000001)
					+ make_mapping(3, 100, 0x310000, "/bin/bash", 100));

				// ACT
				r->symbol_name_by_va(0x120060);

				// ASSERT
				assert_equal(1u, requests.size());
				assert_is_false(requests[0].expired());

				// ACT
				r->symbol_name_by_va(0x311000);
				r->symbol_name_by_va(0x210000);

				// ASSERT
				assert_equal(3u, requests.size());
				assert_is_false(requests[0].expired());
				assert_is_false(requests[1].expired());
				assert_is_false(requests[2].expired());
				assert_not_equal(requests[0].lock(), requests[1].lock());
				assert_not_equal(requests[1].lock(), requests[2].lock());
				assert_not_equal(requests[0].lock(), requests[2].lock());

				// ACT (no new request is generated)
				r->symbol_name_by_va(0x210050);

				// ASSERT
				assert_equal(3u, requests.size());

				// ACT
				r.reset();

				// ASSERT
				assert_equal(3u, requests.size());
				assert_is_true(requests[0].expired());
				assert_is_true(requests[1].expired());
				assert_is_true(requests[2].expired());
			}


			test( RequestHandleIsReleasedWhenCallbackIsInvoked )
			{
				// INIT
				auto r = make_shared<symbol_resolver>(modules, mappings);
				vector< weak_ptr<void> > requests;
				vector<tables::modules::metadata_ready_cb> callbacks;
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 11, 121 },
				};

				modules->request_presence = [&] (shared_ptr<void> &req, unsigned, tables::modules::metadata_ready_cb cb) {
					req = make_shared<bool>();
					requests.push_back(req);
					callbacks.push_back(cb);
				};

				add_records_invalidate(*mappings, plural
					+ make_mapping(0, 11, 0x120050, "c:\\dev\\test.exe", 1919191)
					+ make_mapping(1, 12, 0x120100, "nonono", 17));
				r->symbol_name_by_va(0x120060);

				// ACT
				callbacks.back()(create_metadata(symbols));

				// ASSERT
				assert_is_true(requests.back().expired());
			}

		end_test_suite
	}
}
