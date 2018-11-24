#include <frontend/async_symbol_resolver.h>

#include "mocks.h"

#include <map>
#include <ut/assert.h>
#include <ut/test.h>
#include <wpl/base/concepts.h>

using namespace std;

namespace micro_profiler
{
	inline bool operator ==(const symbol_resolver::symbol_t &lhs, const symbol_resolver::symbol_t &rhs)
	{	return lhs.name == rhs.name && lhs.file == rhs.file && lhs.line == rhs.line;	}

	namespace tests
	{
		namespace mocks
		{
			unsigned int g_dummy_count;

			class symbol_resolver : public micro_profiler::symbol_resolver, wpl::noncopyable
			{
			public:
				symbol_resolver(unsigned int &count)
					: _count(count)
				{	++_count;	}

				~symbol_resolver()
				{	--_count;	}

				template <typename T, size_t n>
				void assign_symbols(T (&symbols)[n])
				{	_symbols = map<address_t, symbol_t>(symbols, symbols + n);	}

			public:
				mutable vector<address_t> symbol_log;

			private:
				virtual bool get_symbol(address_t address, symbol_t &symbol) const
				{
					symbol_log.push_back(address);
					return _symbols.find(address) != _symbols.end() ? symbol = _symbols[address], true : false;
				}

				virtual void add_image(const wstring &/*path*/, address_t /*base_address*/)
				{	}

			private:
				unsigned int &_count;
				mutable map<address_t, symbol_t> _symbols;
			};

			class resolver_factory
			{
			public:
				explicit resolver_factory(symbol_resolver **created = 0)
					: _created(created), _count(g_dummy_count)
				{	}

				explicit resolver_factory(unsigned int &count)
					: _created(0), _count(count)
				{	}

				shared_ptr<symbol_resolver> operator ()() const
				{
					shared_ptr<symbol_resolver> p(new symbol_resolver(_count));

					if (_created)
						*_created = p.get();
					return p;
				}

			private:
				const resolver_factory &operator =(const resolver_factory &rhs);

			private:
				symbol_resolver **_created;
				unsigned int &_count;
			};
		}

		begin_test_suite( AsyncSymbolResolverTests )
			unsigned int count;
			shared_ptr<mocks::queue> cradle_queue, client_queue;

			init( CreateQueues )
			{
				count = 0;
				cradle_queue.reset(new mocks::queue);
				client_queue.reset(new mocks::queue);
			}

			test( UnderlyingResolverIsBeingCreatedFromCradleQueue )
			{
				// INIT
				mocks::resolver_factory f(count);

				// INIT / ACT
				async_symbol_resolver r(f, client_queue, cradle_queue);

				// ASSERT
				assert_equal(0u, count);

				// ACT
				cradle_queue->process_all();

				// ASSERT
				assert_equal(1u, count);
			}


			test( UnderlyingResolverIsBeingDestroyedFromCradleQueue )
			{
				// INIT
				auto_ptr<async_symbol_resolver> r(new async_symbol_resolver(mocks::resolver_factory(count),
					client_queue, cradle_queue));

				cradle_queue->process_all();

				// ACT
				r.reset();

				// ASSERT
				assert_equal(1u, count);

				// ACT
				cradle_queue->process_all();

				// ASSERT
				assert_equal(0u, count);
			}


			test( ResolutionRequestProducesDistinctNonNullHandles )
			{
				// INIT
				async_symbol_resolver r(mocks::resolver_factory(), client_queue, cradle_queue);
				shared_ptr<void> req[2];

				// ACT
				r.get_symbol(req[0], 0x12345678, [] (const symbol_resolver::symbol_t &) { });
				r.get_symbol(req[1], 0x87654321, [] (const symbol_resolver::symbol_t &) { });

				// ASSERT
				assert_not_null(req[0]);
				assert_not_null(req[1]);
				assert_not_equal(req[0], req[1]);
			}


			test( TheCallbackIsNotCalledImmediately )
			{
				// INIT
				async_symbol_resolver r(mocks::resolver_factory(), client_queue, cradle_queue);
				shared_ptr<void> req;

				// ACT / ASSERT
				r.get_symbol(req, 0x12345678, [] (const symbol_resolver::symbol_t &) {
					assert_is_false(true);
				});
			}


			test( ResolutionAttemptedAtTheAddressesPassed )
			{
				// INIT
				mocks::symbol_resolver *ur = 0;
				async_symbol_resolver r(mocks::resolver_factory(&ur), client_queue, cradle_queue);
				shared_ptr<void> req[3];

				cradle_queue->process_all();

				// ACT / ASSERT
				r.get_symbol(req[0], 0x12345678, [] (const symbol_resolver::symbol_t &) {	});
				r.get_symbol(req[1], 0x22345678, [] (const symbol_resolver::symbol_t &) {	});

				// ASSERT
				assert_is_empty(ur->symbol_log);

				// ACT
				cradle_queue->process_all();

				// ASSERT
				address_t reference1[] = { 0x12345678, 0x22345678, };
				assert_equal(reference1, ur->symbol_log);

				// ACT
				r.get_symbol(req[2], 0x87654321, [] (const symbol_resolver::symbol_t &) {	});
				cradle_queue->process_all();

				// ASSERT
				address_t reference2[] = { 0x12345678, 0x22345678, 0x87654321, };
				assert_equal(reference2, ur->symbol_log);
			}


			test( ResolutionIsNotAttemptedForAReleasedHandle )
			{
				// INIT
				mocks::symbol_resolver *ur = 0;
				async_symbol_resolver r(mocks::resolver_factory(&ur), client_queue, cradle_queue);
				shared_ptr<void> req;

				cradle_queue->process_all();
				r.get_symbol(req, 0x12345678, [] (const symbol_resolver::symbol_t &) {	});

				// ACT
				req.reset();
				cradle_queue->process_all();

				// ASSERT
				assert_is_empty(ur->symbol_log);
			}


			test( ResolutionIsPassedToCallbackProvided )
			{
				// INIT
				mocks::symbol_resolver *ur = 0;
				async_symbol_resolver r(mocks::resolver_factory(&ur), client_queue, cradle_queue);
				shared_ptr<void> req[3];
				vector<symbol_resolver::symbol_t> results;
				symbol_resolver::symbol_t symbols[] = {
					{ L"Bar", L"/usr/files/x.cpp", 123 },
					{ L"foo", L"/usr/y.cpp", 1230 },
					{ L"bAr", L"/usr/xx/zzz.cpp", 130 },
				};
				pair<address_t, symbol_resolver::symbol_t> addressed_symbols[] = {
					make_pair(0x12345678, symbols[0]), make_pair(0x22345678, symbols[1]), make_pair(0x72345678, symbols[2]),
				};

				cradle_queue->process_all();
				ur->assign_symbols(addressed_symbols);

				// ACT
				r.get_symbol(req[0], 0x12345678, [&results] (const symbol_resolver::symbol_t &r) { results.push_back(r); });
				r.get_symbol(req[1], 0x22345678, [&results] (const symbol_resolver::symbol_t &r) { results.push_back(r); });
				cradle_queue->process_all();

				// ASSERT
				assert_is_empty(results);

				// ACT
				client_queue->process_all();

				// ASSERT
				symbol_resolver::symbol_t reference1[] = { symbols[0], symbols[1] };

				assert_equal(reference1, results);

				// ACT
				r.get_symbol(req[2], 0x72345678, [&results] (const symbol_resolver::symbol_t &r) { results.push_back(r); });
				cradle_queue->process_all();
				client_queue->process_all();

				// ASSERT
				assert_equal(symbols, results);
			}


			test( CallbackIsNotCalledIfHandleIsReleasedAfterSymbolResolved )
			{
				// INIT
				mocks::symbol_resolver *ur = 0;
				async_symbol_resolver r(mocks::resolver_factory(&ur), client_queue, cradle_queue);
				shared_ptr<void> req;
				bool called = false;
				symbol_resolver::symbol_t symbols[] = { { L"Bar", L"/usr/files/x.cpp", 123 }, };
				pair<address_t, symbol_resolver::symbol_t> addressed_symbols[] = { make_pair(0x12345678, symbols[0]), };

				cradle_queue->process_all();
				ur->assign_symbols(addressed_symbols);
				r.get_symbol(req, 0x12345678, [&called] (const symbol_resolver::symbol_t &) { called = true; });
				cradle_queue->process_all();

				// ACT
				req.reset();
				client_queue->process_all();

				// ASSERT
				assert_is_false(called);
			}
		end_test_suite
	}
}
