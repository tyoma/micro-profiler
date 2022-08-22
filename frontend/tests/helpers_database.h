#pragma once

#include <frontend/database.h>
#include <frontend/keyer.h>

namespace micro_profiler
{
	namespace tests
	{
		inline const sdb::immutable_unique_index<sdb::table<tables::module>, keyer::external_id> &modules_by_id(const profiling_session &session)
		{	return sdb::unique_index<keyer::external_id>(session.modules);	}
	}
}
