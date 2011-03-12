#include "repository.h"

#include "fs.h"
#include "mt.h"

#include <fstream>
#include <unordered_map>

using namespace std;
using namespace fs;
using namespace mt;

namespace
{
	class entry
	{
		static filetime parse_filetime(string::const_iterator b, string::const_iterator e)
		{
			string modstamp_str(b, e);

			return modstamp_str != "dummy timestamp" ? parse_ctime_to_filetime(modstamp_str) : dummy_modstamp;
		}

	public:
		static const filetime dummy_modstamp = (filetime)-1;

	public:
		entry(const vector< pair<string::const_iterator, string::const_iterator> > &entry_parts);

		wstring filename;
		filetime modstamp;
	};

	class entries
	{
		typedef unordered_map< wstring, shared_ptr<entry> > entries_by_name;
		
		entries_by_name _entries_by_name;

	public:
		entries(const wstring &entries_path);

		shared_ptr<entry> find_entry(const wstring filename) const;
	};

	class cvs_repository : public repository
	{
		const wstring _root;
		bool _exit_requested;
		listener &_listener;
		shared_ptr<volatile waitable> _monitor;
		auto_ptr<thread> _tracker_thread;

		void track_changes();
		void evaluate_changes_and_notify(const wstring &directory, vector< pair<wstring, repository::state> > &collector);

	public:
		cvs_repository(const wstring &root, listener &l);
		~cvs_repository();

		virtual state get_filestate(const wstring &path) const;
	};


	entry::entry(const vector< pair<string::const_iterator, string::const_iterator> > &entry_parts)
		: filename(entry_parts[1].first, entry_parts[1].second),
			modstamp(parse_filetime(entry_parts[3].first, entry_parts[3].second))
	{	}


	entries::entries(const wstring &entries_path)
	{
		string line;
		ifstream s(entries_path);
		vector< pair<string::const_iterator, string::const_iterator> > entry_parts;

		while (getline(s, line))
		{
			entry_parts.clear();
			split(line.begin(), line.end(), '/', back_inserter(entry_parts));
			if (entry_parts.size() == 6)
			{
				shared_ptr<entry> e(new entry(entry_parts));
			
				_entries_by_name.insert(make_pair(e->filename, e));
			}
		}
	}

	shared_ptr<entry> entries::find_entry(const wstring filename) const
	{
		entries_by_name::const_iterator i = _entries_by_name.find(filename);

		return i != _entries_by_name.end() ? i->second : 0;
	}


	cvs_repository::cvs_repository(const wstring &root, listener &l)
		: _root(root), _exit_requested(false), _listener(l)
	{
		if (get_entry_type(_root / L"cvs/entries") != entry_file)
			throw invalid_argument("");
		_monitor = create_directory_monitor(_root, false);
		_tracker_thread.reset(new thread([&] () { track_changes(); }));
	}

	cvs_repository::~cvs_repository()
	{
		_exit_requested = true;
		_tracker_thread.reset();
	}

	void cvs_repository::track_changes()
	{
		do
		{
			if (waitable::satisfied == _monitor->wait(100))
			{
				vector< pair<wstring, repository::state> > collector;

				evaluate_changes_and_notify(_root, collector);
				_listener.modified(collector);
			}
		}	while (!_exit_requested);
	}

	void cvs_repository::evaluate_changes_and_notify(const wstring &directory, vector< pair<wstring, repository::state> > &collector)
	{
		entries es(directory / L"cvs/entries");

		for (directory_iterator i(directory); i; ++i)
			if ((*i).name != L"cvs")
			{
				shared_ptr<entry> e(es.find_entry((*i).name));

				if (!e)
					collector.push_back(make_pair((*i).name, repository::state_unversioned));
				else if ((*i).modified > e->modstamp)
					collector.push_back(make_pair(e->filename, repository::state_modified));
			}
	}

	repository::state cvs_repository::get_filestate(const wstring &path) const
	{
		filetime modstamp;
		entries es(get_base_directory(path) / L"cvs/entries");
		shared_ptr<entry> e(es.find_entry(get_filename(path)));
		bool exists(get_filetimes(path, 0, &modstamp, 0));

		if (e && exists)
			return e->modstamp == entry::dummy_modstamp ? state_new : modstamp > e->modstamp ? state_modified : state_intact;
		else if (e)
			return state_missing;
		else
			return state_unversioned;
	}
}

shared_ptr<repository> repository::create_cvs_sc(const wstring &root, listener &l)
{	return shared_ptr<repository>(new cvs_repository(root, l));	}
