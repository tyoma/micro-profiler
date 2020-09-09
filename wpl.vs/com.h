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

#include <atlbase.h>
#include <atlcom.h>

namespace wpl
{
	namespace vs
	{
		template <typename BaseT>
		class freethreaded : public BaseT, public IUnknown
		{
		protected:
			typedef freethreaded freethreaded_base;

		protected:
			DECLARE_PROTECT_FINAL_CONSTRUCT()

			BEGIN_COM_MAP(freethreaded)
				COM_INTERFACE_ENTRY(IUnknown)
				COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, _marshaller)
			END_COM_MAP()

			HRESULT FinalConstruct()
			{
				CComPtr<IUnknown> u;
				HRESULT hr = QueryInterface(IID_PPV_ARGS(&u));

				return S_OK == hr ? ::CoCreateFreeThreadedMarshaler(u, &_marshaller) : hr;
			}

		private:
			CComPtr<IUnknown> _marshaller;
		};
	}
}
