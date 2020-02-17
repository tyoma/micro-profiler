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

#include <comdef.h>

namespace micro_profiler
{
	struct dispatch
	{
		static _variant_t get(const IDispatchPtr &object, const wchar_t *name, const _variant_t &index = vtMissing);
		static _variant_t get(const IDispatchPtr &object, DISPID id);
		static _variant_t get_item(const IDispatchPtr &object, const _variant_t &index);
		static void put(const IDispatchPtr &object, const wchar_t *name, const _variant_t &value);
		static _variant_t call(const IDispatchPtr &object, const wchar_t *name);
		static _variant_t call(const IDispatchPtr &object, const wchar_t *name, const _variant_t &arg1);

		template <typename F>
		static void for_each_variant_as_dispatch(const IDispatchPtr &collection, F receiver);
	};



	template <typename F>
	inline void dispatch::for_each_variant_as_dispatch(const IDispatchPtr &collection, F receiver)
	{
		IEnumVARIANTPtr ev(dispatch::get(collection, DISPID_NEWENUM));
		ULONG fetched = 0;

		for (_variant_t v; S_OK == ev->Next(1u, &v, &fetched) && fetched; v.Clear())
		{
			v.ChangeType(VT_DISPATCH);
			receiver(v);
		}
	}
}
