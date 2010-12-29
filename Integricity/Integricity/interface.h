#pragma once

#include "basics.h"

#include <string>
#include <vector>
#include <utility>
#include <memory>

struct source_control : destructible
{
	enum state { state_intact, state_modified, state_new, state_unversioned, state_removed };

	struct listener
	{
		virtual void modified(const std::vector< std::pair<std::wstring, state> > &modifications) = 0;
	};

	virtual state get_filestate(const std::wstring &path) const = 0;

	static std::shared_ptr<source_control> create_cvs_sc(listener &l);
};

