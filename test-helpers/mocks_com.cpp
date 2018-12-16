#include "mocks_com.h"

#include <collector/tests/mocks.h>
#include <common/noncopyable.h>
#include <windows.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			namespace
			{
				void add(unsigned *value, int addendum)
				{	*value += addendum;	}

				class frontend : public ISequentialStream, noncopyable
				{
				public:
					frontend(const shared_ptr<frontend_state> &state)
						: _references(0), _state(state)
					{
						if (_state->constructed)
							_state->constructed();
					}

					~frontend()
					{
						if (_state->destroyed)
							_state->destroyed();
					}

					// IUnknown methods
					virtual STDMETHODIMP QueryInterface(REFIID riid, void **object)
					{
						if (IID_IUnknown == riid)
							*object = static_cast<IUnknown *>(this);
						else if (IID_ISequentialStream == riid)
							*object = static_cast<ISequentialStream *>(this);
						else
							return E_NOINTERFACE;
						AddRef();
						return S_OK;
					}

					virtual STDMETHODIMP_(ULONG) AddRef()
					{	return ++_references;	}

					virtual STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release()
					{
						unsigned result = --_references;

						if (!result)
							delete this;
						return result;
					}

					// ISequentialStream methods
					STDMETHODIMP Read(void *, ULONG, ULONG *)
					{	return S_OK;	}

					STDMETHODIMP Write(const void *, ULONG, ULONG *)
					{	return S_OK;	}

				private:
					unsigned _references;
					shared_ptr<frontend_state> _state;
				};

				class factory : private IClassFactory, noncopyable
				{
				public:
					factory(const guid_t &id, const shared_ptr<frontend_state> &state)
						: _references(0), _state(state)
					{
						::CoInitialize(0);
						::CoRegisterClassObject(reinterpret_cast<const GUID &>(id), this,
							CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, REGCLS_MULTIPLEUSE, &_cookie);
					}

					~factory()
					{
						::CoRevokeClassObject(_cookie);
						::CoUninitialize();
					}

				private:
					// IUnknown methods
					virtual STDMETHODIMP QueryInterface(REFIID riid, void **object)
					{
						if (IID_IUnknown == riid)
							*object = static_cast<IUnknown *>(this);
						else if (IID_IClassFactory == riid)
							*object = static_cast<IClassFactory *>(this);
						else
							return E_NOINTERFACE;
						AddRef();
						return S_OK;
					}

					virtual STDMETHODIMP_(ULONG) AddRef()
					{	return ++_references;	}

					virtual STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release()
					{	return --_references;	}

					// IClassFactory methods
					virtual STDMETHODIMP CreateInstance(IUnknown * /*outer*/, REFIID riid, void **object)
					{	return (new frontend(_state))->QueryInterface(riid, object);	}

					virtual STDMETHODIMP LockServer(BOOL /*lock*/)
					{	return S_OK;	}

				private:
					DWORD _cookie;
					unsigned _references;
					shared_ptr<frontend_state> _state;
				};
			}

			shared_ptr<void> create_frontend_factory(const guid_t &id, unsigned &references)
			{
				shared_ptr<frontend_state> state(new frontend_state);

				state->constructed = bind(&add, &references, +1);
				state->destroyed = bind(&add, &references, -1);
				return create_frontend_factory(id, state);
			}

			shared_ptr<void> create_frontend_factory(const guid_t &id, const shared_ptr<frontend_state> &state)
			{	return shared_ptr<void>(new factory(id, state));	}
		}
	}
}
