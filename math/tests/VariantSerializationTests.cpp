#include <math/variant.h>

#include <test-helpers/helpers.h>
#include <strmd/serializer.h>
#include <strmd/deserializer.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace math
{
	namespace tests
	{
		using namespace micro_profiler::tests;

		namespace
		{
			typedef strmd::varint packer;
		}

		begin_test_suite( VariantSerializationTests )
			test( StoredValueIsSerializedAccordinglyToTheType )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				unsigned type;
				int ivalue;
				string svalue;
				vector<int> vvalue;

				variant< int, variant<string> > v1(1221);
				variant< int, variant<string> > v2((string)"lorem ipsum");
				variant< vector<int>, variant< int, variant<string> > > v3(vector<int>(3, 17));

				// ACT
				s(v1);
				s(v2);
				s(v3);

				// ASSERT
				ds(type);
				ds(ivalue);

				assert_equal(1u, type);
				assert_equal(1221, ivalue);

				ds(type);
				ds(svalue);

				assert_equal(0u, type);
				assert_equal("lorem ipsum", svalue);

				ds(type);
				ds(vvalue);

				int reference[] = {	17, 17, 17,	};

				assert_equal(2u, type);
				assert_equal(reference, vvalue);
			}


			test( VariantValueIsDeserialized )
			{
				// INIT
				vector_adapter buffer;
				strmd::serializer<vector_adapter, packer> s(buffer);
				strmd::deserializer<vector_adapter, packer> ds(buffer);
				variant< int, variant<string> > v1(0);
				variant< vector<int>, variant< int, variant<string> > > v2(0);

				s(variant< int, variant<string> >(1221));
				s(variant< int, variant<string> >((string)"lorem ipsum"));
				s(variant< int, variant<string> >((string)"amet dolor"));
				s(variant< vector<int>, variant< int, variant<string> > >(vector<int>(7, 170)));
				s(variant< vector<int>, variant< int, variant<string> > >(vector<int>(2, 1)));

				// ACT
				ds(v1);

				// ASSERT
				assert_equal((variant< int, variant<string> >(1221)), v1);

				// ACT
				ds(v1);

				// ASSERT
				assert_equal((variant< int, variant<string> >((string)"lorem ipsum")), v1);

				// ACT
				ds(v1);

				// ASSERT
				assert_equal((variant< int, variant<string> >((string)"amet dolor")), v1);

				// ACT
				ds(v2);

				// ASSERT
				assert_equal((variant< vector<int>, variant< int, variant<string> > >(vector<int>(7, 170))), v2);

				// ACT
				ds(v2);

				// ASSERT
				assert_equal((variant< vector<int>, variant< int, variant<string> > >(vector<int>(2, 1))), v2);
			}
		end_test_suite
	}
}
