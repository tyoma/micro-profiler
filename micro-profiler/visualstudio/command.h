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

#include <memory>
#include <string>
#include <wpl/concepts.h>

namespace micro_profiler
{
	template <typename ContextT>
	class command : wpl::noncopyable
	{
	public:
		enum { supported = 0x01, enabled = 0x02, visible = 0x04, checked = 0x08 };
		typedef std::shared_ptr<command> ptr;
		typedef ContextT context_type;

	public:
		command(int id, bool is_group = false);

		virtual bool query_state(const ContextT &context, unsigned item, unsigned &flags) const = 0;
		virtual bool get_name(const ContextT &context, unsigned item, std::wstring &name) const;
		virtual void exec(ContextT &context, unsigned item) = 0;

	public:
		int id;
		bool is_group;
	};

	template <typename ContextT, int cmdid, bool is_group = false>
	struct command_defined : command<ContextT>
	{
		command_defined();
	};



	template <typename ContextT>
	inline command<ContextT>::command(int id_, bool is_group_)
		: id(id_), is_group(is_group_)
	{	}

	template <typename ContextT>
	inline bool command<ContextT>::get_name(const ContextT &/*context*/, unsigned /*item*/, std::wstring &/*name*/) const
	{	return false;	}


	template <typename ContextT, int cmdid, bool is_group>
	inline command_defined<ContextT, cmdid, is_group>::command_defined()
		: command<ContextT>(cmdid, is_group)
	{	}
}
