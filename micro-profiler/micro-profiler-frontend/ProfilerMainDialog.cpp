#include "ProfilerMainDialog.h"

ProfilerMainDialog::ProfilerMainDialog()
{	Create(NULL, 0);	}

ProfilerMainDialog::~ProfilerMainDialog()
{
	if (IsWindow())
		DestroyWindow();
}