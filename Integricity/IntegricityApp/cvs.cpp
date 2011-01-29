#include "repository.h"

#include "fs.h"

#include <fstream>
#include <unordered_map>

using namespace std;
using namespace fs;

namespace
{
	struct entry
	{
	public:
		entry(const vector< pair<string::const_iterator, string::const_iterator> > &entry_parts);

		wstring filename;
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
	public:
		cvs_repository(const wstring &root, listener &l);

		virtual state get_filestate(const wstring &path) const;
	};


	entry::entry(const vector< pair<string::const_iterator, string::const_iterator> > &entry_parts)
		: filename(entry_parts[1].first, entry_parts[1].second)
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
			
			shared_ptr<entry> e(new entry(entry_parts));
			
			_entries_by_name.insert(make_pair(e->filename, e));
		}
	}

	shared_ptr<entry> entries::find_entry(const wstring filename) const
	{
		entries_by_name::const_iterator i = _entries_by_name.find(filename);

		return i != _entries_by_name.end() ? i->second : 0;
	}


	cvs_repository::cvs_repository(const wstring &root, listener &l)
	{
		if (get_entry_type(root / L"cvs/entries") != entry_file)
			throw invalid_argument("");
	}

	repository::state cvs_repository::get_filestate(const wstring &path) const
	{
		entry_type et(get_entry_type(path));
		entries es(get_base_directory(path) / L"cvs/entries");
		shared_ptr<entry> e(es.find_entry(get_filename(path)));

		return e == 0 ? state_unversioned :
			et != entry_none ? state_intact : state_missing;
	}
}

shared_ptr<repository> repository::create_cvs_sc(const wstring &root, listener &l)
{	return shared_ptr<repository>(new cvs_repository(root, l));	}
