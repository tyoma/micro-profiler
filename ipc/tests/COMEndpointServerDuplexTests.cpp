#include <ipc/endpoint_com.h>

#include "mocks.h"

#include <atlbase.h>
#include <atlcom.h>
#include <common/string.h>
#include <test-helpers/com.h>
#include <test-helpers/helpers.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	using namespace tests;

	namespace ipc
	{
		namespace tests
		{
			namespace
			{
				template <typename T>
				CComPtr< CComObject<T> > construct()
				{
					CComObject<T> *p;

					if (E_OUTOFMEMORY == CComObject<T>::CreateInstance(&p) || !p)
						throw bad_alloc();
					return CComPtr< CComObject<T> > (p);
				}

				CLSID convert(const guid_t &id)
				{	return reinterpret_cast<const CLSID &>(id);	}

				class wrong_sink : public CComObjectRoot, public IUnknown
				{
				public:
					BEGIN_COM_MAP(wrong_sink)
						COM_INTERFACE_ENTRY(IUnknown)
					END_COM_MAP()
				};

				class valid_sink : public CComObjectRoot, public ISequentialStream
				{
				public:
					BEGIN_COM_MAP(valid_sink)
						COM_INTERFACE_ENTRY(ISequentialStream)
					END_COM_MAP()

				public:
					valid_sink()
						: _destroyed(0)
					{	}

					void set_destroyed(bool &destroyed)
					{	_destroyed = &destroyed;	}

					void FinalRelease()
					{
						if (_destroyed)
							*_destroyed = true;
					}

					STDMETHODIMP Read(void * /*buffer*/, ULONG /*size*/, ULONG * /*read*/)
					{	return E_NOTIMPL;	}

					STDMETHODIMP Write(const void *buffer, ULONG size, ULONG * /*written*/)
					{
						messages.push_back(vector<byte>(static_cast<const byte *>(buffer), static_cast<const byte *>(buffer)
							+ size));
						return S_OK;
					}

				public:
					vector< vector<byte> > messages;

				private:
					bool *_destroyed;
				};
			}

			begin_test_suite( COMEndpointServerDuplexTests )
				guid_t ids[2];
				auto_ptr<com_initialize> initializer;
				shared_ptr<mocks::server> server_factory;

				init( Initialize )
				{
					ids[0] = micro_profiler::tests::generate_id();
					ids[1] = micro_profiler::tests::generate_id();
					initializer.reset(new com_initialize);
					server_factory.reset(new mocks::server);
				}


				test( ServerSessionExposesConnectionPoint )
				{
					// INIT / ACT
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);

					// ACT / ASSERT
					CComPtr<IUnknown> server;

					server.CoCreateInstance(convert(ids[0]));

					CComQIPtr<IConnectionPoint> server_cp(server);
					IID iid;
					DWORD cookie;
					CComPtr< CComObject<wrong_sink> > sink = construct<wrong_sink>();

					assert_not_null(server_cp);
					assert_equal(E_POINTER, server_cp->GetConnectionInterface(0));
					assert_equal(S_OK, server_cp->GetConnectionInterface(&iid));
					assert_equal(IID_ISequentialStream, iid);
					assert_equal(E_NOTIMPL, server_cp->EnumConnections(0));
					assert_equal(E_NOTIMPL, server_cp->GetConnectionPointContainer(0));
					assert_equal(E_POINTER, server_cp->Advise(0, 0));
					assert_equal(E_INVALIDARG, server_cp->Advise(0, &cookie));
					assert_equal(E_NOINTERFACE, server_cp->Advise(sink, &cookie));
				}


				test( SinkCanOnlyBeAdvisedOnce )
				{
					// INIT / ACT
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);
					CComPtr<IConnectionPoint> server;
					CComPtr< CComObject<valid_sink> > sink = construct<valid_sink>();
					DWORD cookie = 0;

					server.CoCreateInstance(convert(ids[0]));

					// ACT / ASSERT
					assert_equal(S_OK, server->Advise(sink, &cookie));

					// ASSERT
					assert_equal(1u, cookie);

					// INIT
					cookie = 12345678;

					// ACT / ASSERT
					assert_equal(CONNECT_E_ADVISELIMIT, server->Advise(sink, &cookie));

					// ASSERT
					assert_equal(12345678u, cookie);
				}


				test( SinkIsHeldWhendAdvised )
				{
					// INIT / ACT
					bool destroyed = false;
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);
					CComPtr<IConnectionPoint> server;
					CComPtr< CComObject<valid_sink> > sink = construct<valid_sink>();
					DWORD cookie = 0;

					sink->set_destroyed(destroyed);

					server.CoCreateInstance(convert(ids[0]));
					server->Advise(sink, &cookie);

					// ACT
					sink = 0;

					// ASSERT
					assert_is_false(destroyed);

					// ACT / ASSERT
					assert_equal(CONNECT_E_NOCONNECTION, server->Unadvise(0));
					assert_equal(CONNECT_E_NOCONNECTION, server->Unadvise(123));
					assert_equal(S_OK, server->Unadvise(cookie));

					// ASSERT
					assert_is_true(destroyed);
				}


				test( CannotUnunadviseIfNoConnection )
				{
					// INIT / ACT
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);
					CComPtr<IConnectionPoint> server;

					server.CoCreateInstance(convert(ids[0]));

					// ACT / ASSERT
					assert_equal(CONNECT_E_NOCONNECTION, server->Unadvise(1));
				}


				test( SendingOutboundDataCallsSuppliedStream )
				{
					// INIT
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);
					CComPtr<IConnectionPoint> server;
					const byte d1[] = "some text passed in the buffer...";
					const byte d2[] = "November Rains";
					CComPtr< CComObject<valid_sink> > sink = construct<valid_sink>();
					DWORD cookie;

					// ACT
					server.CoCreateInstance(convert(ids[0]));
					server->Advise(sink, &cookie);

					// ASSERT
					assert_not_null(server_factory->sessions[0]->outbound);

					// ACT
					server_factory->sessions[0]->outbound->message(mkrange(d1));

					// ASSERT
					assert_equal(1u, sink->messages.size());
					assert_equal(d1, sink->messages[0]);

					// ACT
					server_factory->sessions[0]->outbound->message(mkrange(d2));

					// ASSERT
					assert_equal(2u, sink->messages.size());
					assert_equal(d2, sink->messages[1]);
				}


				test( AttemptToWriteWhenConnectionPointIsNotReadyFails )
				{
					// INIT
					shared_ptr<void> h = com::run_server(to_string(ids[0]).c_str(), server_factory);
					CComPtr<IConnectionPoint> server;
					const byte d1[] = "some text passed in the buffer...";
					CComPtr< CComObject<valid_sink> > sink = construct<valid_sink>();

					// ACT
					server.CoCreateInstance(convert(ids[0]));

					// ASSERT
					assert_not_null(server_factory->sessions[0]->outbound);

					// ACT / ASSERT
					assert_throws(server_factory->sessions[0]->outbound->message(mkrange(d1)), runtime_error);
				}

			end_test_suite
		}
	}
}
