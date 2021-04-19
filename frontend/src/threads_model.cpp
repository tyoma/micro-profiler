#include <frontend/threads_model.h>

#include <common/formatting.h>

#pragma warning(disable: 4355)

using namespace std;

namespace micro_profiler
{
	namespace
	{
		double to_seconds(mt::milliseconds v)
		{	return v.count() * 0.001;	}

		struct trackable : wpl::trackable
		{
			template <typename U>
			trackable(U &u, index_type index)
				: _underlying(index ? u.track(index - 1) : shared_ptr<const wpl::trackable>())
			{	}

			virtual index_type index() const override
			{	return _underlying ? _underlying->index() + 1 : 0;	}

			shared_ptr<const wpl::trackable> _underlying;
		};
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

		return end() != i ? native_id = static_cast<unsigned int>(i->second.native_id), true : false;
	}

	bool threads_model::get_key(unsigned int &thread_id, index_type index) const throw()
	{	return index >= 1 && index < get_count() ? thread_id = _view[index - 1].first, true : false;	}

	threads_model::index_type threads_model::get_count() const throw()
	{	return _view.get_count() + 1;	}

	void threads_model::get_value(index_type index, string &text) const
	{
		if (index == 0)
		{
			text = "All Threads";
		}
		else
		{
			const thread_info &v = _view[index - 1].second;

			text = "#", itoa<10>(text, v.native_id);
			if (!v.description.empty())
				text += " - " + v.description;
			text += " - CPU: ", format_interval(text, to_seconds(v.cpu_time));
			text += ", started: +", format_interval(text, to_seconds(v.start_time));
			if (v.complete)
				text += ", ended: +", format_interval(text, to_seconds(v.end_time));
		}
	}

	shared_ptr<const wpl::trackable> threads_model::track(index_type index) const
	{	return shared_ptr<trackable>(new trackable(_view, index));	}
}
