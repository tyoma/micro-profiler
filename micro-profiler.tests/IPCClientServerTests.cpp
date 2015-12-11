#include <ipc/client.h>
#include <ipc/server.h>

#include <wpl/mt/thread.h>

#include "assert.h"

namespace std
{
	using tr1::bind;
	using namespace tr1::placeholders;
}

using namespace std;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace micro_profiler
{
	namespace ipc
	{
		namespace tests
		{
			namespace
			{
				typedef vector< vector<byte> > buffers;

				template <typename T, size_t size>
				vector<T> mkvector(T (&array_ptr)[size])
				{	return vector<T>(array_ptr, array_ptr + size);	}

				void dummy(const vector<byte> &/*input*/, vector<byte> &/*output*/)
				{	}

				void process_message(const vector<byte> &input, vector<byte> &output, buffers &inputs, buffers &outputs)
				{
					inputs.push_back(input);
					output = outputs.back();
					outputs.pop_back();
				}
			}

			[TestClass]
			public ref class IPCClientServerTests
			{
			public: 
				[TestMethod]
				void NoServersAreListedIfNoneIsRunning()
				{
					// INIT
					vector<string> servers;

					// ACT
					client::enumerate_servers(servers);

					// ASSERT
					Assert::IsTrue(servers.empty());
				}


				[TestMethod]
				void CreatedServersAreListed()
				{
					// INIT
					vector<string> servers;

					// ACT
					server s1("foo", &dummy);
					client::enumerate_servers(servers);

					// ASSERT
					const string reference1[] = { "foo", };

					assert::sequences_equivalent(reference1, servers);

					// ACT
					server s2("Foe X", &dummy);
					client::enumerate_servers(servers);

					// ASSERT
					const string reference2[] = { "foo", "Foe X", };

					assert::sequences_equivalent(reference2, servers);

					// ACT
					server s3("Bar Z", &dummy), s4("hope.this.will.be.visible", &dummy);
					client::enumerate_servers(servers);

					// ASSERT
					const string reference3[] = { "foo", "Foe X", "Bar Z", "hope.this.will.be.visible", };

					assert::sequences_equivalent(reference3, servers);
				}


				[TestMethod]
				void DeletedServersAreRemovedFromList()
				{
					// INIT
					vector<string> servers;
					auto_ptr<server> s1(new server("foo2", &dummy)), s2(new server("bar", &dummy)), s3(new server("bzzz. . .p", &dummy));

					// ACT
					s2.reset();
					client::enumerate_servers(servers);

					// ASSERT
					const string reference1[] = { "foo2", "bzzz. . .p", };

					assert::sequences_equivalent(reference1, servers);

					// ACT
					s1.reset();
					client::enumerate_servers(servers);

					// ASSERT
					const string reference2[] = { "bzzz. . .p", };

					assert::sequences_equivalent(reference2, servers);
				}


				[TestMethod]
				void ClientReadsServersOutputs()
				{
					// INIT
					buffers inputs, outputs;

					byte m1[] = "hello!";
					byte m2[] = "how are you?";
					byte m3[] = "bye!";

					outputs.push_back(mkvector(m3));
					outputs.push_back(mkvector(m2));
					outputs.push_back(mkvector(m1));

					server s("test-1", bind(&process_message, _1, _2, ref(inputs), ref(outputs)));
					client c("test-1");
					vector<byte> i, o;

					// ACT / ASSERT (must not hang)
					c.call(i, o);

					// ASSERT
					assert::sequences_equivalent(m1, o);

					// ACT
					c.call(i, o);

					// ASSERT
					assert::sequences_equivalent(m2, o);

					// ACT
					c.call(i, o);

					// ASSERT
					assert::sequences_equivalent(m3, o);
				}


				[TestMethod]
				void ServerReadsClientsInputs()
				{
					// INIT
					buffers inputs, outputs(10);

					byte m1[] = "hello!";
					byte m2[] = "how are you?";
					byte m3[] = "bye!";

					server s("test-2", bind(&process_message, _1, _2, ref(inputs), ref(outputs)));
					client c("test-2");
					vector<byte> o;

					// ACT
					c.call(mkvector(m1), o);
					c.call(mkvector(m3), o);

					// ASSERT
					Assert::AreEqual(2u, inputs.size());
					assert::sequences_equivalent(m1, inputs[0]);
					assert::sequences_equivalent(m3, inputs[1]);

					// ACT
					c.call(mkvector(m2), o);

					// ASSERT
					Assert::AreEqual(3u, inputs.size());
					assert::sequences_equivalent(m2, inputs[2]);
				}


				[TestMethod]
				void ServerCanReadsLongMessagesFromClients()
				{
					// INIT
					buffers inputs, outputs(10);

					server s("test-3", bind(&process_message, _1, _2, ref(inputs), ref(outputs)));
					client c("test-3");
					vector<byte> i, o;

					i.resize(122131);
					i[3] = 89;
					i[30101] = 190;
					i[122130] = 17;

					// ACT
					c.call(i, o);

					// ASSERT
					Assert::AreEqual(1u, inputs.size());
					assert::sequences_equivalent(i, inputs[0]);

					// INIT
					i.resize(13739);
					i[3] = 89;
					i[7122] = 191;
					i[13738] = 19;

					// ACT
					c.call(i, o);

					// ASSERT
					Assert::AreEqual(2u, inputs.size());
					assert::sequences_equivalent(i, inputs[1]);
				}


				[TestMethod]
				void ClientCanReadLongServerReplies()
				{
					// INIT
					buffers inputs, outputs(10);

					server s("test-3", bind(&process_message, _1, _2, ref(inputs), ref(outputs)));
					client c("test-3");
					vector<byte> i, o, tmp;

					tmp.resize(122133);
					tmp[3] = 79;
					tmp[3011] = 90;
					tmp[122121] = 171;
					outputs.push_back(tmp);

					// ACT
					c.call(i, o);

					// ASSERT
					assert::sequences_equivalent(tmp, o);

					// INIT
					tmp.resize(7356);
					tmp[3] = 89;
					tmp[7100] = 191;
					outputs.push_back(tmp);

					// ACT
					c.call(i, o);

					// ASSERT
					assert::sequences_equivalent(tmp, o);
				}
			};
		}
	}
}
