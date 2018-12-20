//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "frontend.h"

#include <resources/resource.h>

#include <atlbase.h>
#include <atlcom.h>

namespace micro_profiler
{
	class frontend_manager_impl;

	class ATL_NO_VTABLE Frontend : public ISequentialStream, public CComObjectRootEx<CComSingleThreadModel>,
		public CComCoClass<Frontend>, public frontend_impl
	{
	public:
		DECLARE_REGISTRY_RESOURCEID(IDR_PROFILER_FRONTEND)
		DECLARE_CLASSFACTORY_EX(frontend_manager_impl)

		BEGIN_COM_MAP(Frontend)
			COM_INTERFACE_ENTRY(ISequentialStream)
		END_COM_MAP()

	public:
		void disconnect() throw()
		{	::CoDisconnectObject(this, 0);	}

		STDMETHODIMP Read(void *, ULONG, ULONG *)
		{	return E_NOTIMPL;	}

		STDMETHODIMP Write(const void *message_, ULONG size, ULONG * /*written*/)
		{
			message(const_byte_range(static_cast<const byte *>(message_), size));
			return S_OK;
		}
	};
}
