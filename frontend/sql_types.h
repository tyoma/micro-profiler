#pragma once

namespace micro_profiler
{
	namespace sql
	{
		template <typename T, typename F>
		struct primary_key
		{
			F T::*field;
		};



		template <typename T, typename F>
		inline primary_key<T, F> pk(F T::*field)
		{
			primary_key<T, F> result = {	field	};
			return result;
		}
	}
}
