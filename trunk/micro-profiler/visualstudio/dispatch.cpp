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

#include "dispatch.h"

#include <stdexcept>

using namespace std;

namespace micro_profiler
{
	namespace integration
	{
		dispatch::dispatch(const _variant_t &result)
			: _underlying(result)
		{	}

		dispatch::dispatch(const IDispatchPtr &object)
			: _underlying(object, true)
		{	}

		IDispatchPtr dispatch::this_object() const
		{
			if (IDispatchPtr o = _underlying)
				return o;
			throw runtime_error("The result is not an object!");
		}

		DISPID dispatch::name_id(const wchar_t *name) const
		{
			DISPID id;
			LPOLESTR names[] = { const_cast<LPOLESTR>(name), };

			if (S_OK == this_object()->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id))
				return id;
			throw runtime_error(static_cast<const char *>(_bstr_t(L"The name '") + name + L"' was not found for the object!"));
		}

		HRESULT dispatch::invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters, _variant_t &result) const
		{
			UINT invalidArg = 0;

			return this_object()->Invoke(name ? name_id(name) : 0, IID_NULL, LOCALE_USER_DEFAULT, wFlags,
				&parameters, &result, NULL, &invalidArg);
		}

		_variant_t dispatch::invoke(const wchar_t *name, WORD wFlags, DISPPARAMS &parameters) const
		{
			_variant_t result;

			if (S_OK == invoke(name, wFlags, parameters, result))
				return result;
			throw runtime_error("Failed invoking method!");
		}

		dispatch dispatch::get(const wchar_t *name, const _variant_t &index) const
		{
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&index), NULL, index != vtMissing ? 1 : 0, 0 };

			return dispatch(invoke(name, DISPATCH_PROPERTYGET, dispparams));
		}

		void dispatch::put(const wchar_t *name, const _variant_t &value) const
		{
			DISPID propputnamed = DISPID_PROPERTYPUT;
			DISPPARAMS dispparams = { const_cast<_variant_t *>(&value), &propputnamed, 1, 1 };

			invoke(name, DISPATCH_PROPERTYPUT, dispparams);
		}

		dispatch dispatch::operator()(const wchar_t *name)
		{
			DISPPARAMS dispparams = { 0 };

			return dispatch(invoke(name, DISPATCH_METHOD, dispparams));
		}
	}
}
