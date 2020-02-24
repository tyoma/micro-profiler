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

#pragma once

#include <common/formatting.h>
#include <common/noncopyable.h>

#include <memory>
#include <string>
#include <vector>

namespace micro_profiler
{
	namespace log
	{
		struct logger;

		typedef std::vector<char> buffer_t;
		typedef std::shared_ptr<logger> logger_ptr;

		enum level { severe, info };

		struct attribute
		{
			const char *name;
			virtual void format_value(buffer_t &buffer) const = 0;
		};

		template <typename T>
		class ref_attribute_impl : public attribute
		{
		public:
			ref_attribute_impl(const char *name_, const T &value);

		private:
			const ref_attribute_impl &operator =(const ref_attribute_impl &rhs);
			virtual void format_value(buffer_t &buffer) const;

		private:
			const T &_value;
		};

		class e/*vent*/ : noncopyable
		{
		public:
			e/*vent*/(logger &l, const char *message, level level_ = info);
			~e/*vent*/();

			template <typename T>
			const e/*vent*/ &operator %(const T &value) const;
			const e/*vent*/ &operator %(const attribute &a) const;

		private:
			logger &_logger;
		};

		struct logger
		{
			virtual void begin(const char *message, level level_) throw() = 0;
			virtual void add_attribute(const attribute &a) throw() = 0;
			virtual void commit() throw() = 0;
		};



		inline void to_string(buffer_t &buffer, char value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, unsigned char value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, short value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, unsigned short value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, int value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, unsigned int value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, long int value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, unsigned long int value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, long long int value) {	itoa<10>(buffer, value);	}
		inline void to_string(buffer_t &buffer, unsigned long long int value) {	itoa<10>(buffer, value);	}

		void to_string(buffer_t &buffer, bool value);
		void to_string(buffer_t &buffer, const void *value);
		void to_string(buffer_t &buffer, const char *value);
		void to_string(buffer_t &buffer, const std::string &value);

		template <typename T>
		inline ref_attribute_impl<T> a(const char *name, const T &value)
		{	return ref_attribute_impl<T>(name, value);	}


		template <typename T>
		inline ref_attribute_impl<T>::ref_attribute_impl(const char *name_, const T &value)
			: _value(value)
		{	name = name_;	}

		template <typename T>
		inline void ref_attribute_impl<T>::format_value(buffer_t &buffer) const
		{	log::to_string(buffer, _value);	}


		inline e/*vent*/::e(logger &l, const char *message, level level_)
			: _logger(l)
		{	_logger.begin(message, level_);	}

		inline e/*vent*/::~e()
		{	_logger.commit();	}

		template <typename T>
		inline const e/*vent*/ &e/*vent*/::operator %(const T &value) const
		{
			static char empty = 0;
			return *this % static_cast<const attribute &>(a(&empty, value));
		}

		inline const e/*vent*/ &e/*vent*/::operator %(const attribute &a_) const
		{	return _logger.add_attribute(a_), *this;	}


		extern logger_ptr g_logger;
	}
}

#define A(__a) static_cast<const micro_profiler::log::attribute &>(micro_profiler::log::a(#__a, __a))
#define LOG(__message) micro_profiler::log::e(*micro_profiler::log::g_logger, (__message), micro_profiler::log::info)
#define LOGE(__message) micro_profiler::log::e(*micro_profiler::log::g_logger, (__message), micro_profiler::log::severe)
