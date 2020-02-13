#include <frontend/symbol_resolver.h>

#include <frontend/serialization.h>

#include <common/protocol.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		begin_test_suite( SymbolResolverTests )
			test( EmptyNameIsReturnedWhenNoMetadataIsLoaded )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x00F));
				assert_is_empty(r->symbol_name_by_va(0x100));
			}


			test( EmptyNameIsReturnedForFunctionsBeforeTheirBegining )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };
				module_info_metadata metadata = { mkvector(symbols), };

				r->add_metadata(basic, metadata);

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x00F));
				assert_is_empty(r->symbol_name_by_va(0x100));
			}


			test( EmptyNameIsReturnedForFunctionsPassTheEnd )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };
				module_info_metadata metadata = { mkvector(symbols), };

				r->add_metadata(basic, metadata);

				// ACT / ASSERT
				assert_is_empty(r->symbol_name_by_va(0x013));
				assert_is_empty(r->symbol_name_by_va(0x107));
			}


			test( FunctionsLoadedThroughAsMetadataAreSearchableByAbsoluteAddress )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = { { "foo", 0x1010, 3 }, { "bar_2", 0x1101, 5 }, };
				module_info_metadata metadata = { mkvector(symbols), };

				// ACT
				r->add_metadata(basic, metadata);

				// ASSERT
				assert_equal("foo", r->symbol_name_by_va(0x1010));
				assert_equal("foo", r->symbol_name_by_va(0x1012));
				assert_equal("bar_2", r->symbol_name_by_va(0x1101));
				assert_equal("bar_2", r->symbol_name_by_va(0x1105));
			}

			test( FunctionsLoadedThroughAsMetadataAreOffsetAccordingly )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { 0, 0, "", { 0x1100000, }, };
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, };
				module_info_metadata metadata = { mkvector(symbols), };

				// ACT
				r->add_metadata(basic, metadata);

				// ASSERT
				assert_equal("foo", r->symbol_name_by_va(0x1100010));
				assert_equal("bar", r->symbol_name_by_va(0x1100101));

				// INIT
				basic.load_address = 0x1100050;

				// ACT
				r->add_metadata(basic, metadata);

				// ASSERT
				assert_equal("foo", r->symbol_name_by_va(0x1100010));
				assert_equal("bar", r->symbol_name_by_va(0x1100101));

				assert_equal("foo", r->symbol_name_by_va(0x1100060));
				assert_equal("bar", r->symbol_name_by_va(0x1100151));
			}


			test( ConstantReferenceFromResolverIsTheSame )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = { { "foo", 0x010, 3 }, { "bar", 0x101, 5 }, { "baz", 0x108, 5 }, };
				module_info_metadata metadata = { mkvector(symbols), };

				r->add_metadata(basic, metadata);

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
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				symbol_resolver::fileline_t dummy;

				// ACT / ASSERT
				assert_is_false(r->symbol_fileline_by_va(0, dummy));
			}


			test( FileLineInformationIsReturnedBySymolProvider )
			{
				// INIT
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 0, 11, 121 },
					{ "b", 0x101, 5, 0, 1, 1 },
					{ "c", 0x158, 5, 0, 7, 71 },
					{ "d", 0x188, 5, 0, 7, 713 },
				};
				pair<unsigned, string> files[] = {
					make_pair(11, "zoo.cpp"), make_pair(7, "c:/umi.cpp"), make_pair(1, "d:\\dev\\kloo.cpp"),
				};
				module_info_metadata metadata = { mkvector(symbols), mkvector(files) };
				symbol_resolver::fileline_t results[4];

				r->add_metadata(basic, metadata);

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
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols1[] = {
					{ "a", 0x010, 3, 0, 11, 121 },
					{ "b", 0x101, 5, 0, 1, 1 },
					{ "c", 0x158, 5, 0, 7, 71 },
					{ "d", 0x188, 5, 0, 7, 713 },
				};
				symbol_info symbols2[] = {
					{ "a", 0x010, 3, 0, 1, 121 },
					{ "b", 0x101, 5, 0, 1, 1 },
					{ "c", 0x158, 5, 0, 7, 71 },
					{ "d", 0x180, 5, 0, 50, 71 },
				};
				pair<unsigned, string> files1[] = {
					make_pair(11, "zoo.cpp"), make_pair(1, "c:/umi.cpp"), make_pair(7, "d:\\dev\\kloo.cpp"),
				};
				pair<unsigned, string> files2[] = {
					make_pair(1, "ZOO.cpp"), make_pair(7, "/projects/umi.cpp"), make_pair(50, "d:\\dev\\abc.cpp"),
				};
				module_info_metadata metadata[] = { 
					{ mkvector(symbols1), mkvector(files1) },
					{ mkvector(symbols2), mkvector(files2) },
				};
				symbol_resolver::fileline_t results[8];

				r->add_metadata(basic, metadata[0]);
				basic.load_address = 0x8000;

				// ACT / ASSERT
				r->add_metadata(basic, metadata[1]);
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


			test( SymbolResolverIsSerializable )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> ser(buffer);
				strmd::deserializer<vector_adapter, packer> dser(buffer);
				shared_ptr<symbol_resolver> r(new symbol_resolver);
				mapped_module basic = { };
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 0, 11, 121 },
					{ "b", 0x101, 5, 0, 1, 1 },
					{ "cccc", 0x158, 5, 0, 7, 71 },
					{ "ddd", 0x188, 5, 0, 7, 713 },
				};
				pair<unsigned, string> files[] = {
					make_pair(11, "zoo.cpp"), make_pair(1, "c:/umi.cpp"), make_pair(7, "d:\\dev\\kloo.cpp"),
				};
				module_info_metadata metadata = { mkvector(symbols), mkvector(files) };
				symbol_resolver::fileline_t results[4];

				r->add_metadata(basic, metadata);

				// ACT
				shared_ptr<symbol_resolver> r2(new symbol_resolver);

				ser(*r);
				dser(*r2);

				// ASSERT
				assert_equal("a", r2->symbol_name_by_va(0x010));
				assert_equal("b", r2->symbol_name_by_va(0x101));
				assert_equal("cccc", r2->symbol_name_by_va(0x158));
				assert_equal("ddd", r2->symbol_name_by_va(0x188));

				assert_is_true(r2->symbol_fileline_by_va(0x010, results[0]));
				assert_is_true(r2->symbol_fileline_by_va(0x101, results[1]));
				assert_is_true(r2->symbol_fileline_by_va(0x158, results[2]));
				assert_is_true(r2->symbol_fileline_by_va(0x188, results[3]));

				symbol_resolver::fileline_t reference[] = {
					make_pair("zoo.cpp", 121),
					make_pair("c:/umi.cpp", 1),
					make_pair("d:\\dev\\kloo.cpp", 71),
					make_pair("d:\\dev\\kloo.cpp", 713),
				};

				assert_equal(reference, results);
			}


			test( FilelineIsNotAvailableForMissingFunctions )
			{
				// INIT
				symbol_resolver r;
				symbol_resolver::fileline_t result;

				// ACT / ASSERT
				assert_is_false(r.symbol_fileline_by_va(123, result));
			}


			test( FilelineIsNotAvailableForFunctionsWithMissingFiles )
			{
				// INIT
				symbol_resolver r;
				symbol_info symbols[] = {
					{ "a", 0x010, 3, 0, 11, 121 },
					{ "b", 0x101, 5, 0, 1, 1 },
					{ "cccc", 0x158, 5, 0, 0, 71 },
					{ "ddd", 0x188, 5, 0, 0, 713 },
				};
				mapped_module basic = { };
				module_info_metadata metadata = { mkvector(symbols), };
				symbol_resolver::fileline_t result;

				r.add_metadata(basic, metadata);

				// ACT / ASSERT
				assert_is_false(r.symbol_fileline_by_va(0x010, result));
				assert_is_false(r.symbol_fileline_by_va(0x101, result));
				assert_is_false(r.symbol_fileline_by_va(0x158, result));
				assert_is_false(r.symbol_fileline_by_va(0x188, result));
			}


		end_test_suite
	}
}
