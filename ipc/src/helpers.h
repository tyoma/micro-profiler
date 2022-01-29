//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <string>
#include <stdexcept>

namespace micro_profiler
{
	namespace ipc
	{
		template <typename FactoryT>
		struct constructor
		{
			std::string protocol;
			FactoryT constructor_method;
		};



		template <typename FactoryT, size_t n>
		inline const FactoryT &select(constructor<FactoryT> (&constructors)[n], const std::string &typed_endpoint_id,
			std::string &endpoint_id)
		{
			const size_t delim = typed_endpoint_id.find('|');
			if (delim == std::string::npos)
				throw std::invalid_argument(typed_endpoint_id);
			const std::string protocol = typed_endpoint_id.substr(0, delim);

			endpoint_id = typed_endpoint_id.substr(delim + 1);

			for (size_t i = 0; i != n; ++i)
			{
				if (protocol == constructors[i].protocol)
					return constructors[i].constructor_method;
			}
			throw protocol_not_supported(typed_endpoint_id.c_str());
		}
	}
}
