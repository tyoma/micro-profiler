#include <tchar.h>
#include <windows.h>
#include <atlstr.h>

const LPCTSTR c_mailslot_name = _T("\\\\.\\mailslot\\CQG\\ProfilerMailslot");

int _tmain(int argc, const TCHAR *argv[])
{
	HANDLE hmailslot = ::CreateFile(c_mailslot_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	CStringA command;

	if (argc == 2 && CString(argv[1]) == _T("clear"))
		command = _T("CLEAR");
	else if (argc == 3 && CString(argv[1]) == _T("dump"))
		command = CString(_T("DUMP ")) + argv[2];

	DWORD dummy;

	if (command.GetLength())
		::WriteFile(hmailslot, (LPCSTR)command, command.GetLength() + 1, &dummy, NULL);

	::CloseHandle(hmailslot);
	return 0;
}
