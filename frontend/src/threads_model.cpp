#include <frontend/threads_model.h>

#include <common/formatting.h>
#include <common/string.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
	namespace
	{
		double to_seconds(mt::milliseconds v)
		{	return v.count() * 0.001;	}
	}

	threads_model::threads_model(const request_threads_t &requestor)
		: _requestor(requestor), _view(*this)
	{
		_view.set_order([] (unsigned int, const thread_info &lhs, unsigned int, const thread_info &rhs) {
			return lhs.cpu_time < rhs.cpu_time;
		}, false);
	}

	bool threads_model::get_native_id(unsigned int &native_id, unsigned int thread_id) const throw()
	{
		const const_iterator i = find(thread_id);

		return end() != i ? native_id = i->second.native_id, true : false;
	}

	bool threads_model::get_key(unsigned int &thread_id, index_type index) const throw()
	{	return index >= 1 && index < get_count() ? thread_id = _view[index - 1].first, true : false;	}

	threads_model::index_type threads_model::get_count() const throw()
	{	return _view.size() + 1;	}

	void threads_model::get_text(index_type index, wstring &text) const
	{
		if (index == 0)
		{
			text = L"All Threads";
		}
		else
		{
			const thread_info &v = _view[index - 1].second;

			text = L"#", itoa<10>(text, v.native_id);
			if (!v.description.empty())
				text += L" - " + unicode(v.description);
			text += L" - CPU: ", format_interval(text, to_seconds(v.cpu_time));
			text += L", started: +", format_interval(text, to_seconds(v.start_time));
			if (v.complete)
				text += L", ended: +", format_interval(text, to_seconds(v.end_time));
		}
	}
}
