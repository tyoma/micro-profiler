#include <visualstudio/command-target.h>

#include <guiddef.h>
#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace micro_profiler
{
	namespace tests
	{
		namespace
		{
			extern const GUID c_guid1 = { 0x12345678, 0x000, 0x000, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };
			extern const GUID c_guid2 = { 0x22345678, 0x000, 0x000, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };
			extern const GUID c_guid3 = { 0x32345678, 0x000, 0x000, { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 } };

			template <typename ContextT, const GUID *CommandSetID>
			class CommandTargetFinal : public CommandTarget<ContextT, CommandSetID>
			{
			public:
				template <typename IteratorT>
				CommandTargetFinal(IteratorT begin, IteratorT end)
					: CommandTarget<ContextT, CommandSetID>(begin, end)
				{	}

			public:
				ContextT context;

			private:
				STDMETHODIMP QueryInterface(REFIID /*iid*/, void ** /*ppv*/) {	throw 0;	}
				STDMETHODIMP_(ULONG) AddRef() {	throw 0;	}
				STDMETHODIMP_(ULONG) Release() {	throw 0;	}

			private:
				virtual ContextT get_context()
				{	return context;	}
			};

			struct mock_command : command<int>
			{
				mock_command(int id, unsigned count = 1)
					: command<int>(id, count > 1 || count == 0), state(0), group_count(count), executed(false)
				{	}

				virtual bool query_state(const int &, unsigned item, unsigned &flags) const
				{ return flags = state, item < group_count; }

				virtual bool get_name(const int &, unsigned item, wstring &name) const
				{	return item < names.size() ? name = names[item], true : false;	}

				virtual void exec(int &, unsigned item)
				{	executed = true, item_index = item;	}

				int state;
				unsigned group_count;
				bool executed;
				unsigned item_index;
				vector<wstring> names;
			};

			template <typename T>
			struct mock_command_t : command<T>
			{
				mock_command_t(int id_)
					: command<T>(id_)
				{	}

				virtual bool query_state(const T &context_, unsigned /*item*/, unsigned &/*flags*/) const
				{ return context = context_, true; }

				virtual bool get_name(const T &context_, unsigned /*item*/, wstring &/*name*/) const
				{	return context = context_, false;	}

				virtual void exec(T &context_, unsigned /*item*/)
				{	context = context_;	}

				mutable T context;
			};

			struct throwing_command : command<int>
			{
				throwing_command()
					: command<int>(1)
				{	}

				virtual bool query_state(const int &/*context*/, unsigned /*item*/, unsigned &/*flags*/) const
				{	throw 0;	}

				virtual bool get_name(const int &, unsigned /*item*/, wstring &/*name*/) const
				{	return false;	}

				virtual void exec(int &, unsigned /*item*/)
				{	throw 0;	}
			};
		}

		begin_test_suite( CommandTargetTests )
			test( CommandTargetRejectsProcessingOfCommandsFromUnknownSets )
			{
				// INIT
				const command<int>::ptr c = command<int>::ptr(new mock_command(0));
				CommandTargetFinal<int, &c_guid1> ct1(&c, &c + 1);
				IOleCommandTarget &ict1 = ct1;
				CommandTargetFinal<int, &c_guid2> ct2(&c, &c + 1);
				IOleCommandTarget &ict2 = ct2;
				OLECMD dummy[1];

				// ACT / ASSERT
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict1.QueryStatus(&c_guid2, 1, dummy, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict1.QueryStatus(&c_guid3, 1, dummy, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict2.QueryStatus(&c_guid1, 1, dummy, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict2.QueryStatus(&c_guid3, 1, dummy, NULL));

				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict1.Exec(&c_guid2, 1, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict1.Exec(&c_guid3, 1, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict2.Exec(&c_guid1, 1, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
				assert_equal(OLECMDERR_E_UNKNOWNGROUP, ict2.Exec(&c_guid3, 1, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
			}


			static int getSingleCommandState(IOleCommandTarget &target, const GUID &command_set, unsigned command)
			{
				OLECMD cmd = { command, 0 };

				assert_equal(S_OK, target.QueryStatus(&command_set, 1, &cmd, NULL));
				return cmd.cmdf;
			}

			test( CommandStateIsResponded )
			{
				// INIT
				const shared_ptr<mock_command> c = shared_ptr<mock_command>(new mock_command(11));
				CommandTargetFinal<int, &c_guid1> ct(&c, &c + 1);

				c->state = 0;

				// ACT / ASSERT
				assert_equal(OLECMDF_DEFHIDEONCTXTMENU | OLECMDF_INVISIBLE,
					getSingleCommandState(ct, c_guid1, 11));

				// INIT
				c->state = command<int>::supported;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_DEFHIDEONCTXTMENU | OLECMDF_INVISIBLE,
					getSingleCommandState(ct, c_guid1, 11));

				// INIT
				c->state = command<int>::supported | command<int>::enabled;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_DEFHIDEONCTXTMENU | OLECMDF_INVISIBLE,
					getSingleCommandState(ct, c_guid1, 11));

				// INIT
				c->state = command<int>::supported | command<int>::visible;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED, getSingleCommandState(ct, c_guid1, 11));

				// INIT
				c->state = command<int>::supported | command<int>::visible | command<int>::checked;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_LATCHED, getSingleCommandState(ct, c_guid1, 11));
			}


			test( CommandIsSelectedByID )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(11)),
					shared_ptr<mock_command>(new mock_command(13)),
					shared_ptr<mock_command>(new mock_command(19)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));

				c[1]->state = command<int>::supported | command<int>::visible | command<int>::enabled;
				c[2]->state = command<int>::supported | command<int>::visible | command<int>::checked;

				// ACT / ASSERT
				assert_equal(OLECMDF_DEFHIDEONCTXTMENU | OLECMDF_INVISIBLE, getSingleCommandState(ct, c_guid1, 10));
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, getSingleCommandState(ct, c_guid1, 11));
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_LATCHED, getSingleCommandState(ct, c_guid1, 13));
				assert_equal(OLECMDF_DEFHIDEONCTXTMENU | OLECMDF_INVISIBLE, getSingleCommandState(ct, c_guid1, 19));
			}


			test( MissingCommandsAreNotSupported )
			{
				// INIT
				const shared_ptr<mock_command> c = shared_ptr<mock_command>(new mock_command(11));
				CommandTargetFinal<int, &c_guid2> ct(&c, &c + 1);
				OLECMD cmd = { 10, 10000 };

				// ACT / ASSERT
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.QueryStatus(&c_guid2, 1, &cmd, NULL));

				// ASSERT
				assert_equal(0u, cmd.cmdf);

				// INIT
				cmd.cmdID = 12;
				cmd.cmdf = 123;

				// ACT / ASSERT
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.QueryStatus(&c_guid2, 1, &cmd, NULL));

				// ASSERT
				assert_equal(0u, cmd.cmdf);
			}


			test( GroupCommandIsSelectedByRange )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31, 17)),
					shared_ptr<mock_command>(new mock_command(130, 2)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));
				OLECMD cmd = { 48, 10000 };

				c[0]->state = command<int>::supported;
				c[1]->state = command<int>::supported | command<int>::enabled | command<int>::visible;
				c[2]->state = command<int>::supported | command<int>::visible;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, getSingleCommandState(ct, c_guid1, 31));
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, getSingleCommandState(ct, c_guid1, 35));
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, getSingleCommandState(ct, c_guid1, 47));
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.QueryStatus(&c_guid1, 1, &cmd, NULL));

				// ASSERT
				assert_equal(0u, cmd.cmdf);

				// INIT
				cmd.cmdID = 132;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED, getSingleCommandState(ct, c_guid1, 130));
				assert_equal(OLECMDF_SUPPORTED, getSingleCommandState(ct, c_guid1, 131));
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.QueryStatus(&c_guid1, 1, &cmd, NULL));
			}


			test( EmptyGroupIsRespondedAsOKButDisabledAndInvisible )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31, 0)),
					shared_ptr<mock_command>(new mock_command(130, 2)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));

				c[1]->state = command<int>::supported | command<int>::enabled | command<int>::visible;

				// ACT / ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_INVISIBLE | OLECMDF_DEFHIDEONCTXTMENU, getSingleCommandState(ct, c_guid1, 31));
			}


			test( SeveralCommandsCanBeProcessed )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31, 17)),
					shared_ptr<mock_command>(new mock_command(130, 2)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));
				OLECMD cmd[3] = { 0 };

				c[0]->state = command<int>::supported;
				c[1]->state = command<int>::supported | command<int>::enabled | command<int>::visible;
				c[2]->state = command<int>::supported | command<int>::visible;

				cmd[0].cmdID = 10;
				cmd[1].cmdID = 33;

				// ACT / ASSERT
				assert_equal(S_OK, ct.QueryStatus(&c_guid1, 2, cmd, NULL));

				// ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_INVISIBLE | OLECMDF_DEFHIDEONCTXTMENU, (int)cmd[0].cmdf);
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, (int)cmd[1].cmdf);

				// INIT
				cmd[0].cmdID = 34;
				cmd[1].cmdID = 10;
				cmd[2].cmdID = 131;

				// ACT / ASSERT
				assert_equal(S_OK, ct.QueryStatus(&c_guid1, 3, cmd, NULL));

				// ASSERT
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_ENABLED, (int)cmd[0].cmdf);
				assert_equal(OLECMDF_SUPPORTED | OLECMDF_INVISIBLE | OLECMDF_DEFHIDEONCTXTMENU, (int)cmd[1].cmdf);
				assert_equal(OLECMDF_SUPPORTED, (int)cmd[2].cmdf);
			}


			test( DifferentContextsArePassedInToQueryState )
			{
				// INIT
				shared_ptr< mock_command_t<int> > c1 = shared_ptr< mock_command_t<int> >(new mock_command_t<int>(11));
				CommandTargetFinal<int, &c_guid1> ct1(&c1, &c1 + 1);
				shared_ptr< mock_command_t<string> > c2 = shared_ptr< mock_command_t<string> >(new mock_command_t<string>(12));
				CommandTargetFinal<string, &c_guid2> ct2(&c2, &c2 + 1);

				ct1.context = 13222;
				ct2.context = "foo";

				// ACT
				getSingleCommandState(ct1, c_guid1, 11);
				getSingleCommandState(ct2, c_guid2, 12);

				// ASSERT
				assert_equal(13222, c1->context);
				assert_equal("foo", c2->context);

				// INIT
				ct1.context = 13;
				ct2.context = "bar";

				// ACT
				getSingleCommandState(ct1, c_guid1, 11);
				getSingleCommandState(ct2, c_guid2, 12);

				// ASSERT
				assert_equal(13, c1->context);
				assert_equal("bar", c2->context);
			}


			test( ExceptionsFromCommandAreAbsorbed )
			{
				// INIT
				shared_ptr<throwing_command> c = shared_ptr<throwing_command>(new throwing_command());
				CommandTargetFinal<int, &c_guid1> ct(&c, &c + 1);
				OLECMD cmd = { 1, 0 };

				// ACT / ASSERT
				assert_equal(E_UNEXPECTED, ct.QueryStatus(&c_guid1, 1, &cmd, NULL));
				assert_equal(E_UNEXPECTED, ct.Exec(&c_guid1, 1, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
			}


			test( CommandIsExecutedBasedOnID )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31)),
					shared_ptr<mock_command>(new mock_command(130)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));

				// ACT
				assert_equal(S_OK, ct.Exec(&c_guid1, 10, OLECMDEXECOPT_DODEFAULT, NULL, NULL));

				// ACT / ASSERT
				assert_is_true(c[0]->executed);
				assert_is_false(c[1]->executed);
				assert_is_false(c[2]->executed);

				// ACT
				assert_equal(S_OK, ct.Exec(&c_guid1, 31, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
				assert_equal(S_OK, ct.Exec(&c_guid1, 130, OLECMDEXECOPT_DODEFAULT, NULL, NULL));

				// ACT / ASSERT
				assert_is_true(c[0]->executed);
				assert_is_true(c[1]->executed);
				assert_is_true(c[2]->executed);
			}


			test( MissingCommandIsReported )
			{
				// INIT
				shared_ptr<mock_command> c = shared_ptr<mock_command>(new mock_command(33));
				CommandTargetFinal<int, &c_guid1> ct(&c, &c + 1);

				// ACT / ASSERT
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.Exec(&c_guid1, 32, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
				assert_equal(OLECMDERR_E_NOTSUPPORTED, ct.Exec(&c_guid1, 34, OLECMDEXECOPT_DODEFAULT, NULL, NULL));
			}


			test( ContextIsPassedForExecution )
			{
				// INIT
				shared_ptr< mock_command_t<int> > c1 = shared_ptr< mock_command_t<int> >(new mock_command_t<int>(11));
				CommandTargetFinal<int, &c_guid1> ct1(&c1, &c1 + 1);
				shared_ptr< mock_command_t<string> > c2 = shared_ptr< mock_command_t<string> >(new mock_command_t<string>(12));
				CommandTargetFinal<string, &c_guid2> ct2(&c2, &c2 + 1);

				ct1.context = 1322;
				ct2.context = "fooE";

				// ACT
				ct1.Exec(&c_guid1, 11, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
				ct2.Exec(&c_guid2, 12, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(1322, c1->context);
				assert_equal("fooE", c2->context);

				// INIT
				ct1.context = 13;
				ct2.context = "bar";

				// ACT
				ct1.Exec(&c_guid1, 11, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
				ct2.Exec(&c_guid2, 12, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(13, c1->context);
				assert_equal("bar", c2->context);
			}


			test( GroupCommandIsSelectedByRangeForExecution )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31, 17)),
					shared_ptr<mock_command>(new mock_command(130, 2)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));

				// ACT / ASSERT
				ct.Exec(&c_guid1, 31, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(0u, c[1]->item_index);

				// ACT / ASSERT
				ct.Exec(&c_guid1, 32, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(1u, c[1]->item_index);

				// ACT / ASSERT
				ct.Exec(&c_guid1, 47, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(16u, c[1]->item_index);

				// ACT / ASSERT
				ct.Exec(&c_guid1, 131, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

				// ASSERT
				assert_equal(1u, c[2]->item_index);
			}


			test( GetingTheCaptionFillsTheBufferProvidedWithZeroTermination )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31, 2)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));
				const size_t buffer_size = 50;
				wchar_t buffer[sizeof(OLECMDTEXT) + (buffer_size - 1) * sizeof(wchar_t)] = { 0 };
				OLECMDTEXT *cmdtext = reinterpret_cast<OLECMDTEXT *>(buffer);
				OLECMD cmd = { };

				cmdtext->cmdtextf = OLECMDTEXTF_NAME;
				cmdtext->cwBuf = 20;
				c[0]->names.push_back(L"a short name");
				c[1]->names.push_back(L"Lorem ipsum dolor sit amet, consectetur adipiscing elit");
				c[1]->names.push_back(L"medium-sized strings are good for your health");

				// ACT
				cmd.cmdID = 10;
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal(L"a short name", wstring(cmdtext->rgwz));
				assert_equal(static_cast<ULONG>(c[0]->names[0].size() + 1), cmdtext->cwActual);

				// ACT
				cmd.cmdID = 31;
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal(L"Lorem ipsum dolor s", wstring(cmdtext->rgwz));
				assert_equal(static_cast<ULONG>(c[1]->names[0].size() + 1), cmdtext->cwActual);

				// ACT
				cmdtext->cwBuf = 22;
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal(L"Lorem ipsum dolor sit", wstring(cmdtext->rgwz));
				assert_equal(static_cast<ULONG>(c[1]->names[0].size() + 1), cmdtext->cwActual);

				// ACT
				cmdtext->cwBuf = 46;
				cmd.cmdID = 32;
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal(L"medium-sized strings are good for your health", wstring(cmdtext->rgwz));
				assert_equal(static_cast<ULONG>(c[1]->names[1].size() + 1), cmdtext->cwActual);
			}


			test( OnlyTheTextForTheFirstCommandIsReturned )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));
				const size_t buffer_size = 50;
				wchar_t buffer[sizeof(OLECMDTEXT) + (buffer_size - 1) * sizeof(wchar_t)] = { 0 };
				OLECMDTEXT *cmdtext = reinterpret_cast<OLECMDTEXT *>(buffer);
				OLECMD cmd[] = { { 31, }, { 10, }, };

				cmdtext->cmdtextf = OLECMDTEXTF_NAME;
				c[0]->names.push_back(L"a short name");
				c[1]->names.push_back(L"Lorem ipsum dolor sit amet, consectetur adipiscing elit");

				// ACT
				cmdtext->cwBuf = 23;
				ct.QueryStatus(&c_guid1, _countof(cmd), cmd, cmdtext);

				// ASSERT
				assert_equal(L"Lorem ipsum dolor sit ", wstring(cmdtext->rgwz));
			}


			test( MissingDynamicTextSetsNameStringToZero )
			{
				// INIT
				const shared_ptr<mock_command> c[] = {
					shared_ptr<mock_command>(new mock_command(10)),
					shared_ptr<mock_command>(new mock_command(31)),
				};
				CommandTargetFinal<int, &c_guid1> ct(c, c + _countof(c));
				const size_t buffer_size = 50;
				wchar_t buffer[sizeof(OLECMDTEXT) + (buffer_size - 1) * sizeof(wchar_t)] = { 0 };
				OLECMDTEXT *cmdtext = reinterpret_cast<OLECMDTEXT *>(buffer);
				OLECMD cmd[] = { { 31, }, { 10, }, };

				cmdtext->cmdtextf = OLECMDTEXTF_NAME;

				// ACT
				cmdtext->cwBuf = 23;
				cmdtext->cwActual = 1542231;
				ct.QueryStatus(&c_guid1, _countof(cmd), cmd, cmdtext);

				// ASSERT
				assert_equal(0u, cmdtext->cwActual);
			}


			test( ContextIsPassedWhenQueryingName )
			{
				// INIT
				shared_ptr< mock_command_t<string> > c = shared_ptr< mock_command_t<string> >(new mock_command_t<string>(12));
				CommandTargetFinal<string, &c_guid1> ct(&c, &c + 1);
				const size_t buffer_size = 50;
				wchar_t buffer[sizeof(OLECMDTEXT) + (buffer_size - 1) * sizeof(wchar_t)] = { 0 };
				OLECMDTEXT *cmdtext = reinterpret_cast<OLECMDTEXT *>(buffer);
				OLECMD cmd = { 12, };

				// ACT
				ct.context = "sdsdf";
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal("sdsdf", c->context);

				// ACT
				ct.context = "something else";
				ct.QueryStatus(&c_guid1, 1, &cmd, cmdtext);

				// ASSERT
				assert_equal("something else", c->context);
			}
		end_test_suite
	}
}
