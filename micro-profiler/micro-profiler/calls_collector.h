#pragma once

namespace micro_profiler
{
#pragma pack(push, 4)
	struct call_record
	{
		void *callee;	// call address + 5 bytes
		unsigned __int64 timestamp;
	};
#pragma pack(pop)

	class __declspec(dllexport) calls_collector
	{
		static calls_collector *_instance;

	public:
		struct acceptor
		{
			virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count) = 0;
		};

	public:
		calls_collector();
		~calls_collector();

		static calls_collector *instance();

		void read_collected(acceptor &a);

		static void enter(call_record call);
		static void exit(call_record call);
	};
}
