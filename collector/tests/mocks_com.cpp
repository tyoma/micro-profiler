#include "mocks_com.h"

#include <windows.h>
#include <wpl/base/concepts.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace mocks
		{
			namespace
			{
				class frontend : public ISequentialStream, wpl::noncopyable
				{
				public:
					frontend(unsigned &referneces)
						: _my_references(0), _references(referneces)
					{	}

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
					{	return ++_references, ++_my_references;	}

					virtual STDMETHODIMP_(ULONG) STDMETHODCALLTYPE Release()
					{
						unsigned result = --_my_references;

						--_references;
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
					unsigned _my_references, &_references;
				};

				class factory : private IClassFactory, wpl::noncopyable
				{
				public:
					factory(const guid_t &id, unsigned &references)
						: _references(references)
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
					{	return (new frontend(_references))->QueryInterface(riid, object);	}

					virtual STDMETHODIMP LockServer(BOOL /*lock*/)
					{	return S_OK;	}

				private:
					DWORD _cookie;
					unsigned &_references;
				};
			}

			shared_ptr<void> create_frontend_factory(const guid_t &id, unsigned &references)
			{	return shared_ptr<void>(new factory(id, references));	}
		}
	}
}
