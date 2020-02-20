//	Copyright (c) 2011-2020 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <logger/log.h>

#include <algorithm>
#include <string.h>

using namespace std;

namespace micro_profiler
{
	namespace log
	{
		namespace
		{
			struct nil_logger : logger
			{
				virtual void begin(const char * /*message*/, level /*level_*/) throw() {	}
				virtual void add_attribute(const attribute &/*a*/) throw() {	}
				virtual void commit() throw() {	}
			};



			template <typename T>
			void uitoa(buffer_t &buffer, T value, int base = 10)
			{
				size_t l0 = buffer.size();

				do
					buffer.push_back("0123456789ABCDEF"[value % base]);
				while (value /= T(base), value);
				reverse(buffer.begin() + l0, buffer.end());
			}

			template <typename T>
			void itoa(buffer_t &buffer, T value, int base = 10)
			{
				if (value < 0)
					buffer.push_back('-'), value = -value;
				uitoa(buffer, value, base);
			}
		}

		logger_ptr g_logger(new nil_logger);

		void to_string(buffer_t &buffer, void *value) {	buffer.push_back('0'), buffer.push_back('x'), uitoa(buffer, reinterpret_cast<size_t>(value), 16);	}
		void to_string(buffer_t &buffer, bool value) {	to_string(buffer, value ? "true" : "false");	}
		void to_string(buffer_t &buffer, char value) {	itoa(buffer, value);	}
		void to_string(buffer_t &buffer, unsigned char value) {	uitoa(buffer, value);	}
		void to_string(buffer_t &buffer, short value) {	itoa(buffer, value);	}
		void to_string(buffer_t &buffer, unsigned short value) {	uitoa(buffer, value);	}
		void to_string(buffer_t &buffer, int value) {	itoa(buffer, value);	}
		void to_string(buffer_t &buffer, unsigned int value) {	uitoa(buffer, value);	}
		void to_string(buffer_t &buffer, long int value) {	itoa(buffer, value);	}
		void to_string(buffer_t &buffer, unsigned long int value) {	uitoa(buffer, value);	}
		void to_string(buffer_t &buffer, long long int value) {	itoa(buffer, value);	}
		void to_string(buffer_t &buffer, unsigned long long int value) {	uitoa(buffer, value);	}
		void to_string(buffer_t &buffer, const string &value) {	buffer.insert(buffer.end(), value.begin(), value.end());	}

		void to_string(buffer_t &buffer, const char *value)
		{
			static const char null[] = "<null>";

			value = value ? value : null;

			const size_t pl = buffer.size(), l = strlen(value);

			buffer.resize(pl + l);
			if (l)
				strncpy(&buffer[pl], value, l);
		}

	}
}
