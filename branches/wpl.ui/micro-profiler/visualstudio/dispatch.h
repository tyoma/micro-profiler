//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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
#include <stdexcept>

namespace micro_profiler
{
	namespace integration
	{
		class dispatch
		{
			_variant_t _underlying;

			explicit dispatch(const _variant_t &result);

			IDispatchPtr this_object() const;

			DISPID name_id(const wchar_t *name) const;

			HRESULT invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters, _variant_t &result) const;

			_variant_t invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters) const;

		public:
			explicit dispatch(const IDispatchPtr &object);

			dispatch get(const wchar_t *name, const _variant_t &index = vtMissing) const;
			void put(const wchar_t *name, const _variant_t &value) const;

			template <typename IndexT>
			dispatch operator[](const IndexT &index) const;

			dispatch operator()(const wchar_t *name);

			template <typename Arg1>
			dispatch operator()(const wchar_t *name, const Arg1 &arg1);

			template <typename Arg1, typename Arg2>
			dispatch operator()(const wchar_t *name, const Arg1 &arg1, const Arg2 &arg2);

			template <typename R>
			operator R() const;
		};



		template <typename IndexT>
		inline dispatch dispatch::operator[](const IndexT &index) const
		{
			_variant_t vindex(index), result;
			DISPPARAMS dispparams = { &vindex, NULL, 1, 0 };

			if (S_OK == invoke(NULL, DISPATCH_PROPERTYGET, dispparams, result)
				|| S_OK == invoke(NULL, DISPATCH_METHOD, dispparams, result))
				return dispatch(result);
			throw runtime_error("Accessing an indexed property failed!");
		}

		template <typename Arg1>
		inline dispatch dispatch::operator()(const wchar_t *name, const Arg1 &arg1)
		{
			_variant_t args[] = { arg1, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename Arg1, typename Arg2>
		inline dispatch dispatch::operator()(const wchar_t *name, const Arg1 &arg1, const Arg2 &arg2)
		{
			_variant_t args[] = { arg1, arg2, };
			DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}

		template <typename R>
		inline dispatch::operator R() const
		{	return _underlying;	}
	}
}
