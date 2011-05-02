#pragma once

#include "./../micro-profiler/_generated/microprofilerfrontend_i.h"

#include <tchar.h>
#include <hash_map>
#include <string>
#include <vector>
#include <memory>

class symbol_resolver;

typedef std::basic_string<TCHAR> tstring;

struct function_statistics
{
	function_statistics(const FunctionStatistics &from, const symbol_resolver &resolver);

	tstring name;
	__int64 times_called, inclusive_time, exclusive_time;
};

class statistics
{
	typedef stdext::hash_map<__int64, function_statistics> statistics_map_;
	class dereferencing_wrapper;

	const symbol_resolver &_symbol_resolver;
	std::auto_ptr<dereferencing_wrapper> _predicate;
	statistics_map_ _statistics;
	std::vector<statistics_map_::const_iterator> _sorted_statistics;

	void operator =(const statistics &);

public:
	typedef statistics_map_ statistics_map;
	typedef bool (*sort_predicate)(const function_statistics &lhs, const function_statistics &rhs);

	sort_predicate _current_predicate;
	bool _ascending;

public:
	statistics(const symbol_resolver &resolver);
	virtual ~statistics();

	const function_statistics &at(unsigned int index) const;
	unsigned int size() const;

	void clear();
	void sort(sort_predicate predicate, bool ascending);
	bool update(const FunctionStatistics *data, unsigned int count);
};


class statistics::dereferencing_wrapper
{
	statistics::sort_predicate _base;
	bool _ascending;

public:
	dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending);

	bool operator ()(const statistics::statistics_map::const_iterator &lhs, const statistics::statistics_map::const_iterator &rhs) const;
};
