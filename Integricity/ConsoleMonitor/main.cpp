#include "./../IntegricityApp/repository.h"

#include <conio.h>
#include <iostream>
#include <algorithm>

using namespace std;

class mylistener : public repository::listener
{
	virtual void modified(const vector< pair<wstring, repository::state> > &modifications)
	{
		for_each(modifications.begin(), modifications.end(), [] (const pair<wstring, repository::state> &e) { wcout << e.second << L" " << e.first << endl; });
		wcout << endl;
	}
};

int wmain(int argc, const wchar_t *argv[])
{
	mylistener l;
	shared_ptr<repository> r(repository::create_cvs_sc(argv[1], l));

	getch();
}
