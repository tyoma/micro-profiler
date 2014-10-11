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

#include "object_lock.h"

#include <windows.h>

namespace std
{
	using tr1::bind;
}

using namespace std;
using namespace wpl;

namespace micro_profiler
{
	void LockModule();
	void UnlockModule();

	namespace
	{
		class ROTObjectLock : public IUnknown
		{
			unsigned int _references;
			DWORD _rotid;
			shared_ptr<self_unlockable> _object;
			slot_connection _slot;

			void Revoke();

		public:
			ROTObjectLock(const shared_ptr<self_unlockable> &object);
			~ROTObjectLock();

			void Register();

			STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
			STDMETHODIMP_(ULONG) AddRef();
			STDMETHODIMP_(ULONG) Release();
		};



		ROTObjectLock::ROTObjectLock(const shared_ptr<self_unlockable> &object)
			: _references(0), _rotid(0), _object(object)
		{
			_slot = object->unlock += bind(&ROTObjectLock::Revoke, this);
			LockModule();
		}

		ROTObjectLock::~ROTObjectLock()
		{
			UnlockModule();
		}

		void ROTObjectLock::Register()
		{
			IRunningObjectTable *rot = 0;
			IMoniker *moniker = 0;

			if (S_OK == ::GetRunningObjectTable(0, &rot))
			{
				if (S_OK == ::CreateItemMoniker(OLESTR("!"), OLESTR("olock"), &moniker))
				{
					rot->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, this, moniker, &_rotid);
					moniker->Release();
				}
				rot->Release();
			}
		}

		void ROTObjectLock::Revoke()
		{
			IRunningObjectTable *rot = 0;

			if (S_OK == ::GetRunningObjectTable(0, &rot))
			{
				rot->Revoke(_rotid);
				rot->Release();
			}
		}

		STDMETHODIMP ROTObjectLock::QueryInterface(REFIID riid, void **ppv)
		{
			if (ppv == NULL)
				return E_POINTER;
			if (IID_IUnknown == riid)
			{
				*ppv = this;
				AddRef();
				return S_OK;
			}
			return E_NOINTERFACE;
		}

		STDMETHODIMP_(ULONG) ROTObjectLock::AddRef()
		{
			++_references;
			return 0;
		}

		STDMETHODIMP_(ULONG) ROTObjectLock::Release()
		{
			if (!--_references)
				delete this;
			return 0;
		}
	}

	void lock(const shared_ptr<self_unlockable> &object)
	{
		ROTObjectLock *lock = new ROTObjectLock(object);

		lock->AddRef();
		lock->Register();
		lock->Release();
	}
}
