#pragma once

#include "calls_collector.h"

namespace micro_profiler
{
	class analyzer : public calls_collector::acceptor
	{
	public:
		bool begin() const;
		bool end() const;

		__declspec(dllexport) virtual void accept_calls(unsigned int threadid, const call_record *calls, unsigned int count);
	};


	inline bool analyzer::begin() const
	{	return true;	}

	inline bool analyzer::end() const
	{	return true;	}
}
