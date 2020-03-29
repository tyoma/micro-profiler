#include "mocks.h"

#include <frontend/serialization.h>
#include <iomanip>
#include <sstream>
#include <strmd/deserializer.h>
#include <strmd/serializer.h>
#include <test-helpers/helpers.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			symbol_resolver::symbol_resolver()
				: micro_profiler::symbol_resolver([] (unsigned) {})
			{	}

			const string &symbol_resolver::symbol_name_by_va(long_address_t address) const
			{
				const string &match = micro_profiler::symbol_resolver::symbol_name_by_va(address);

				if (!match.empty())
					return match;

				return _names[address] = to_string_address(address);
			}

			string symbol_resolver::to_string_address(long_address_t value)
			{
				stringstream s;
				s << uppercase << hex << setw(8) << setfill('0') << value;
				return s.str();
			}


			void threads_model::add(unsigned int thread_id, unsigned int native_id, const string &description)
			{
				vector_adapter stream;
				strmd::serializer<vector_adapter> ser(stream);
				strmd::deserializer<vector_adapter> dser(stream);
				pair<unsigned, thread_info> i[] = { pair<unsigned, thread_info>(), };

				i[0].first = thread_id;
				i[0].second.native_id = native_id, i[0].second.description = description;
				ser(mkvector(i));
				dser(static_cast<micro_profiler::threads_model &>(*this));
			}
		}
	}
}
