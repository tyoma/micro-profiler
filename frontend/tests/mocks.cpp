#include "mocks.h"

#include <iomanip>
#include <sstream>

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

			const std::string &symbol_resolver::symbol_name_by_va(long_address_t address) const
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
		}
	}
}


