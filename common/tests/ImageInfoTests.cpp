#include <common/symbol_resolver.h>

#include <common/module.h>
#include <common/path.h>
#include <test-helpers/helpers.h>

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;
using namespace std::placeholders;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			int g_dummy;

			typedef void (*void_f_t)();
			typedef void (get_function_addresses_1_t)(void_f_t &f1, void_f_t &f2);
			typedef void (get_function_addresses_2_t)(void_f_t &f1, void_f_t &f2, void_f_t &f3);
			typedef void (get_function_addresses_3_t)(void_f_t &f);

			void add_function(map<string, symbol_info> &functions, const symbol_info &si)
			{	functions.insert(make_pair(si.name, si));	}
		}

		begin_test_suite( ImageInfoTests )

			wstring image_paths[4];

			init( InitializePaths )
			{
				image_paths[0] = get_module_info(&g_dummy).path.c_str();
				image_paths[1] = image(L"symbol_container_1").absolute_path();
				image_paths[2] = image(L"symbol_container_2").absolute_path();
				image_paths[3] = image(L"symbol_container_3_nosymbols").absolute_path();
			}


			test( LoadImageFailsWhenInvalidModuleSpecified )
			{
				// ACT / ASSERT
				assert_throws(image_info::load(L""), invalid_argument);
				assert_throws(image_info::load(L"missingABCDEFG"), invalid_argument);
			}


			test( CreateResolverForValidImage )
			{
				// ACT / ASSERT
				assert_not_null(image_info::load(image_paths[0].c_str()));
				assert_not_null(image_info::load(image_paths[1].c_str()));
				assert_not_null(image_info::load(image_paths[2].c_str()));
				assert_not_null(image_info::load(image_paths[3].c_str()));
			}


			test( EnumerationReturnsKnownFunctions )
			{
				// INIT
				shared_ptr<image_info> ii[] = {
					image_info::load(image_paths[1].c_str()),
					image_info::load(image_paths[2].c_str()),
				};
				map<string, symbol_info> functions[2];

				// ACT
				ii[0]->enumerate_functions(bind(&add_function, ref(functions[0]), _1));
				ii[1]->enumerate_functions(bind(&add_function, ref(functions[1]), _1));

				// ASSERT
				assert_equal(1u, functions[0].count("get_function_addresses_1"));
				assert_equal(1u, functions[0].count("format_decimal"));

				assert_equal(1u, functions[1].count("get_function_addresses_2"));
				assert_equal(1u, functions[1].count("function_with_a_nested_call_2"));
				assert_equal(1u, functions[1].count("bubble_sort2"));
				assert_equal(1u, functions[1].count("bubble_sort_expose"));
				assert_equal(1u, functions[1].count("guinea_snprintf"));
			}


			test( DataSymbolsAreSkipped )
			{
				// INIT
				shared_ptr<image_info> ii = image_info::load(image_paths[2].c_str());
				map<string, symbol_info> functions;

				// ACT
				ii->enumerate_functions(bind(&add_function, ref(functions), _1));

				// ASSERT
				assert_equal(0u, functions.count("datum1"));
				assert_equal(0u, functions.count("datum2"));
			}


			test( FunctionRVAsAreValid )
			{
				// INIT
				image images[] = {
					image(image_paths[1].c_str()),
					image(image_paths[2].c_str()),
				};
				const size_t base[] = {
					static_cast<size_t>(images[0].load_address()),
					static_cast<size_t>(images[1].load_address()),
				};
				const byte *reference[] = {
					static_cast<const byte *>(images[0].get_symbol_address("get_function_addresses_1")),
					static_cast<const byte *>(images[0].get_symbol_address("format_decimal")),

					static_cast<const byte *>(images[1].get_symbol_address("get_function_addresses_2")),
					static_cast<const byte *>(images[1].get_symbol_address("function_with_a_nested_call_2")),
					static_cast<const byte *>(images[1].get_symbol_address("bubble_sort2")),
					static_cast<const byte *>(images[1].get_symbol_address("bubble_sort_expose")),
					static_cast<const byte *>(images[1].get_symbol_address("guinea_snprintf")),
				};
				shared_ptr<image_info> ii[] = {
					image_info::load(image_paths[1].c_str()),
					image_info::load(image_paths[2].c_str()),
				};
				map<string, symbol_info> functions[2];

				// ACT
				ii[0]->enumerate_functions(bind(&add_function, ref(functions[0]), _1));
				ii[1]->enumerate_functions(bind(&add_function, ref(functions[1]), _1));

				// ASSERT
				assert_equal(reference[0] - base[0], functions[0].find("get_function_addresses_1")->second.body.begin());
				assert_equal(reference[1] - base[0], functions[0].find("format_decimal")->second.body.begin());

				assert_equal(reference[2] - base[1], functions[1].find("get_function_addresses_2")->second.body.begin());
				assert_equal(reference[3] - base[1], functions[1].find("function_with_a_nested_call_2")->second.body.begin());
				assert_equal(reference[4] - base[1], functions[1].find("bubble_sort2")->second.body.begin());
				assert_equal(reference[5] - base[1], functions[1].find("bubble_sort_expose")->second.body.begin());
				assert_equal(reference[6] - base[1], functions[1].find("guinea_snprintf")->second.body.begin());
			}


			test( FunctionSizesAreValid )
			{
				// This test may not pass on all platoforms/compilers. Need a better one.

				// INIT
				shared_ptr<image_info> ii = image_info::load(image_paths[2].c_str());
				map<string, symbol_info> functions;

				// ACT
				ii->enumerate_functions(bind(&add_function, ref(functions), _1));

				// ASSERT
				assert_equal(1u, functions.count("bubble_sort"));

				assert_is_true(0 < functions.find("bubble_sort_expose")->second.body.length());
				assert_is_true(functions.find("bubble_sort_expose")->second.body.length()
					< functions.find("guinea_snprintf")->second.body.length());
				assert_is_true(functions.find("guinea_snprintf")->second.body.length()
					< functions.find("bubble_sort")->second.body.length());
			}
		end_test_suite


		begin_test_suite( OffsetImageInfoTests )
			test( SymbolsEnumeratedAreOffsetAccordinglyToBase )
			{
				// INIT
				image img(L"symbol_container_2");
				shared_ptr<image_info> ii = image_info::load(img.absolute_path());
				map<string, symbol_info> functions_original, functions_offset;

				ii->enumerate_functions(bind(&add_function, ref(functions_original), _1));

				// INIT / ACT
				offset_image_info oii1(ii, 0x123);

				oii1.enumerate_functions(bind(&add_function, ref(functions_offset), _1));

				// ASSERT
				assert_equal(functions_original.find("get_function_addresses_2")->second.body.begin() + 0x123,
					functions_offset.find("get_function_addresses_2")->second.body.begin());
				assert_equal(functions_original.find("get_function_addresses_2")->second.body.length(),
					functions_offset.find("get_function_addresses_2")->second.body.length());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.body.begin() + 0x123,
					functions_offset.find("function_with_a_nested_call_2")->second.body.begin());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.body.length(),
					functions_offset.find("function_with_a_nested_call_2")->second.body.length());
				assert_equal(functions_original.find("bubble_sort_expose")->second.body.begin() + 0x123,
					functions_offset.find("bubble_sort_expose")->second.body.begin());
				assert_equal(functions_original.find("bubble_sort_expose")->second.body.length(),
					functions_offset.find("bubble_sort_expose")->second.body.length());

				// INIT
				functions_offset.clear();

				// INIT / ACT
				offset_image_info oii2(ii, 0x12345678);

				oii2.enumerate_functions(bind(&add_function, ref(functions_offset), _1));

				// ASSERT
				assert_equal(functions_original.find("get_function_addresses_2")->second.body.begin() + 0x12345678,
					functions_offset.find("get_function_addresses_2")->second.body.begin());
				assert_equal(functions_original.find("get_function_addresses_2")->second.body.length(),
					functions_offset.find("get_function_addresses_2")->second.body.length());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.body.begin() + 0x12345678,
					functions_offset.find("function_with_a_nested_call_2")->second.body.begin());
				assert_equal(functions_original.find("function_with_a_nested_call_2")->second.body.length(),
					functions_offset.find("function_with_a_nested_call_2")->second.body.length());
			}
		end_test_suite
	}
}
