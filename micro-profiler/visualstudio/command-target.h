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

#include "command.h"

#include <algorithm>
#include <docobj.h>
#include <vector>

namespace micro_profiler
{
	template <typename ContextT, const GUID *CommandSetID>
	class CommandTarget : public IOleCommandTarget
	{
	protected:
		typedef CommandTarget<ContextT, CommandSetID> command_target_type;

	protected:
		template <typename IteratorT>
		CommandTarget(IteratorT begin, IteratorT end);

		virtual ContextT get_context() = 0;

	public:
		STDMETHODIMP QueryStatus(const GUID *group, ULONG count, OLECMD commands[], OLECMDTEXT *command_text);
		STDMETHODIMP Exec(const GUID *group, DWORD command, DWORD option, VARIANT *in, VARIANT *out);

	private:
		struct command_less;
		typedef std::vector<typename command<ContextT>::ptr> commands_t;

	private:
		static unsigned convert_state(unsigned state);
		typename command<ContextT>::ptr get_command(int id, unsigned &item) const;

	private:
		commands_t _commands;
		std::wstring _cache;
	};

	template <typename ContextT, const GUID *CommandSetID>
	struct CommandTarget<ContextT, CommandSetID>::command_less
	{
		bool operator ()(const typename command<ContextT>::ptr &lhs, const typename command<ContextT>::ptr &rhs) const;
		bool operator ()(int lhs, const typename command<ContextT>::ptr &rhs) const;
		bool operator ()(const typename command<ContextT>::ptr &lhs, int rhs) const;
	};



	template <typename ContextT, const GUID *CommandSetID>
	inline bool CommandTarget<ContextT, CommandSetID>::command_less::operator ()(
		const typename command<ContextT>::ptr &lhs, const typename command<ContextT>::ptr &rhs) const
	{	return lhs->id < rhs->id;	}

	template <typename ContextT, const GUID *CommandSetID>
	inline bool CommandTarget<ContextT, CommandSetID>::command_less::operator ()(int lhs,
		const typename command<ContextT>::ptr &rhs) const
	{	return lhs < rhs->id;	}

	template <typename ContextT, const GUID *CommandSetID>
	inline bool CommandTarget<ContextT, CommandSetID>::command_less::operator ()(
		const typename command<ContextT>::ptr &lhs, int rhs) const
	{	return lhs->id < rhs;	}


	template <typename ContextT, const GUID *CommandSetID>
	template <typename IteratorT>
	inline CommandTarget<ContextT, CommandSetID>::CommandTarget(IteratorT begin, IteratorT end)
		: _commands(begin, end)
	{	std::sort(_commands.begin(), _commands.end(), command_less());	}

	template <typename ContextT, const GUID *CommandSetID>
	inline STDMETHODIMP CommandTarget<ContextT, CommandSetID>::QueryStatus(const GUID *group, ULONG count,
		OLECMD commands[], OLECMDTEXT *command_text)
	try
	{
		if (*group != *CommandSetID)
			return OLECMDERR_E_UNKNOWNGROUP;
		for (ContextT context = get_context(); count--; )
		{
			unsigned item;
			OLECMD &cmd = commands[count];

			cmd.cmdf = 0;
			if (typename command<ContextT>::ptr c = get_command(cmd.cmdID, item))
			{
				unsigned state;

				if (!c->query_state(context, item, state))
				{
					if (item)
						return OLECMDERR_E_NOTSUPPORTED;
					state = command<ContextT>::supported;
				}
				cmd.cmdf = convert_state(state);
				if (command_text)
				{
					if (c->get_name(context, item, _cache))
					{
						wcsncpy(command_text->rgwz, _cache.c_str(), command_text->cwBuf - 1);
						command_text->rgwz[command_text->cwBuf - 1] = L'\0';
						command_text->cwActual = static_cast<ULONG>(_cache.size() + 1);
					}
					else
					{
						command_text->cwActual = 0;
					}
				}
			}
			else
			{
				return OLECMDERR_E_NOTSUPPORTED;
			}
		}
		return S_OK;
	}
	catch (...)
	{
		return E_UNEXPECTED;
	}

	template <typename ContextT, const GUID *CommandSetID>
	inline STDMETHODIMP CommandTarget<ContextT, CommandSetID>::Exec(const GUID *group, DWORD cmdid, DWORD /*option*/,
		VARIANT * /*in*/, VARIANT * /*out*/)
	try
	{
		if (*group != *CommandSetID)
			return OLECMDERR_E_UNKNOWNGROUP;

		unsigned item;
		ContextT ctx = get_context();
		typename command<ContextT>::ptr c = get_command(cmdid, item);

		return c ? c->exec(ctx, item), S_OK : OLECMDERR_E_NOTSUPPORTED;
	}
	catch (...)
	{
		return E_UNEXPECTED;
	}

	template <typename ContextT, const GUID *CommandSetID>
	inline unsigned CommandTarget<ContextT, CommandSetID>::convert_state(unsigned state)
	{
		return (state & command<ContextT>::supported ? OLECMDF_SUPPORTED : 0)
			| (state & command<ContextT>::enabled ? OLECMDF_ENABLED : 0)
			| (state & command<ContextT>::visible ? 0 : OLECMDF_INVISIBLE | OLECMDF_DEFHIDEONCTXTMENU)
			| (state & command<ContextT>::checked ? OLECMDF_LATCHED : 0);
	}

	template <typename ContextT, const GUID *CommandSetID>
	inline typename command<ContextT>::ptr CommandTarget<ContextT, CommandSetID>::get_command(int id, unsigned &item) const
	{
		commands_t::const_iterator i = std::upper_bound(_commands.begin(), _commands.end(), id, command_less());

		return i != _commands.begin() && ((*--i)->is_group || (*i)->id == id) ? item = id - (*i)->id, *i
			: typename command<ContextT>::ptr();
	}
}
