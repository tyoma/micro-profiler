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

#include "async.h"

#include <wpl.vs/com.h>

#include <atlcom.h>
#include <stdexcept>

using namespace std;

namespace wpl
{
	namespace vs
	{
		namespace async
		{
			namespace
			{
				class task_body : public freethreaded< CComObjectRootEx<CComMultiThreadModel> >, public IVsTaskBody
				{
				public:
					basic_task_function_t task;

				protected:
					BEGIN_COM_MAP(task_body)
						COM_INTERFACE_ENTRY(IVsTaskBody)
						COM_INTERFACE_ENTRY_CHAIN(freethreaded_base)
					END_COM_MAP()

				private:
					STDMETHODIMP DoWork(IVsTask * /*task*/, DWORD dependents_count, IVsTask *dependents[], VARIANT *result)
					try
					{
						*result = task(dependents, dependents_count).Detach();
						return S_OK;
					}
					catch (...)
					{
						return E_FAIL;
					}
				};
			}



			CComPtr<IVsTaskBody> wrap_task(const basic_task_function_t &task)
			{
				CComObject<task_body> *p = 0;

				p->CreateInstance(&p);
				p->task = task;
				return CComPtr<IVsTaskBody>(p);
			}

			CComPtr<IVsTask> when_complete(IVsTask *parent, VSTASKRUNCONTEXT context, const task_function_t &completion)
			{
				CComPtr<IVsTask> child;

				parent->ContinueWith(context, wrap_task([completion] (IVsTask *t[], unsigned c) -> _variant_t {
					if (c == 1)
					{
						_variant_t r;

						t[0]->GetResult(&r);
						return completion(r);
					}
					throw runtime_error("Unexpected continuation state!");
				}), &child);
				return child;
			}
		}
	}
}
