#include "statistics.h"

#include "symbol_resolver.h"
#include <algorithm>

using namespace std;


class statistics::dereferencing_wrapper
{
	statistics::sort_predicate _base;
	bool _ascending;

public:
	dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending);

	bool operator ()(const statistics::statistics_map::const_iterator &lhs, const statistics::statistics_map::const_iterator &rhs) const;
};


function_statistics::function_statistics(const FunctionStatistics &from, const symbol_resolver &resolver)
	: name(resolver.symbol_name_by_va(from.FunctionAddress)), times_called(from.TimesCalled),
		inclusive_time(from.InclusiveTime), exclusive_time(from.ExclusiveTime)
{	}


statistics::dereferencing_wrapper::dereferencing_wrapper(const statistics::sort_predicate &p, bool ascending)
	: _base(p), _ascending(ascending)
{	}

bool statistics::dereferencing_wrapper::operator ()(const statistics::statistics_map::const_iterator &lhs, const statistics::statistics_map::const_iterator &rhs) const
{	return _ascending ? _base(lhs->second, rhs->second) : _base(rhs->second, lhs->second);	}


statistics::statistics(const symbol_resolver &resolver)
	: _symbol_resolver(resolver)
{	}

statistics::~statistics()
{	}

const function_statistics &statistics::at(unsigned int index) const
{	return _sorted_statistics.at(index)->second;	}

unsigned int statistics::size() const
{	return _sorted_statistics.size();	}

void statistics::clear()
{
	_sorted_statistics.clear();
	_statistics.clear();
}

void statistics::sort(sort_predicate predicate, bool ascending)
{
	auto_ptr<dereferencing_wrapper> p(new dereferencing_wrapper(predicate, ascending));

	std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *p);
	_predicate = p;
}

bool statistics::update(const FunctionStatistics *data, unsigned int count)
{
	bool new_insertions(false);

	for (; count; --count, ++data)
	{
		statistics_map::iterator match(_statistics.find(data->FunctionAddress));

		if (match == _statistics.end())
		{
			match = _statistics.insert(make_pair(data->FunctionAddress, function_statistics(*data, _symbol_resolver))).first;
			new_insertions = true;
		}
		else
		{
			match->second.times_called += data->TimesCalled;
			match->second.exclusive_time += data->ExclusiveTime;
			match->second.inclusive_time += data->InclusiveTime;
		}
	}

	if (new_insertions)
	{
		_sorted_statistics.clear();
		for (statistics_map::const_iterator i = _statistics.begin(); i != _statistics.end(); ++i)
			_sorted_statistics.push_back(i);
	}
	if (_predicate.get())
		std::sort(_sorted_statistics.begin(), _sorted_statistics.end(), *_predicate);
	return new_insertions;
}
