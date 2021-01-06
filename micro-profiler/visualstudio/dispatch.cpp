//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
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
	namespace
	{
		DISPID name_id(const IDispatchPtr &object, const wchar_t *name)
		{
			DISPID id;
			LPOLESTR names[] = { const_cast<LPOLESTR>(name), };

			if (S_OK == object->GetIDsOfNames(IID_NULL, names, 1, LOCALE_USER_DEFAULT, &id))
				return id;
			throw runtime_error(static_cast<const char *>(_bstr_t(L"The name '") + name + L"' was not found for the object!"));
		}

		HRESULT invoke(const IDispatchPtr &object, const wchar_t *name, WORD wFlags, DISPPARAMS &parameters, _variant_t &result)
		{
			UINT invalidArg = 0;

			return object->Invoke(name ? name_id(object, name) : 0, IID_NULL, LOCALE_USER_DEFAULT, wFlags,
				&parameters, &result, NULL, &invalidArg);
		}

		_variant_t invoke(const IDispatchPtr &object, const wchar_t *name, WORD wFlags, DISPPARAMS &parameters)
		{
			_variant_t result;

			if (S_OK == invoke(object, name, wFlags, parameters, result))
				return result;
			throw runtime_error("Failed invoking method!");
		}
	}

	_variant_t dispatch::get(const IDispatchPtr &object, const wchar_t *name, const _variant_t &index)
	{
		DISPPARAMS dispparams = { const_cast<_variant_t *>(&index), NULL, index != vtMissing ? 1u : 0u, 0u };

		return invoke(object, name, DISPATCH_PROPERTYGET, dispparams);
	}

	_variant_t dispatch::get(const IDispatchPtr &object, DISPID id)
	{
		_variant_t result;
		DISPPARAMS dispparams = { NULL, NULL, 0u, 0u };
		UINT invalidArg = 0;

		if (S_OK == object->Invoke(id, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD | DISPATCH_PROPERTYGET, &dispparams, &result, NULL, &invalidArg))
			return result;
		throw runtime_error("Cannot call the get-method by ID!");
	}

	_variant_t dispatch::get_item(const IDispatchPtr &object, const _variant_t &index)
	{
		_variant_t vindex(index), result;
		DISPPARAMS dispparams = { &vindex, NULL, 1, 0 };

		if (S_OK == invoke(object, NULL, DISPATCH_METHOD | DISPATCH_PROPERTYGET, dispparams, result))
			return result;
		throw runtime_error("Accessing an indexed property failed!");
	}

	void dispatch::put(const IDispatchPtr &object, const wchar_t *name, const _variant_t &value)
	{
		DISPID propputnamed = DISPID_PROPERTYPUT;
		DISPPARAMS dispparams = { const_cast<_variant_t *>(&value), &propputnamed, 1u, 1u };

		invoke(object, name, DISPATCH_PROPERTYPUT, dispparams);
	}

	_variant_t dispatch::call(const IDispatchPtr &object, const wchar_t *name)
	{
		DISPPARAMS dispparams = { };

		return invoke(object, name, DISPATCH_METHOD, dispparams);
	}

	_variant_t dispatch::call(const IDispatchPtr &object, const wchar_t *name, const _variant_t &arg1)
	{
		_variant_t args[] = { arg1, };
		DISPPARAMS dispparams = { args, NULL, _countof(args), 0 };

		return invoke(object, name, DISPATCH_METHOD, dispparams);
	}
}
