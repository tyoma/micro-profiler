#include "interface.h"

#include "utilities.h"

using namespace std;

class cvs_repository : public repository
{
public:
	cvs_repository(const wstring &root, listener &l)
	{
		throw invalid_argument("");
	}

	virtual state get_filestate(const std::wstring &path) const
	{
		throw 0;
	}
};

shared_ptr<repository> repository::create_cvs_sc(const wstring &root, listener &l)
{
	return shared_ptr<repository>(new cvs_repository(root, l));
}
