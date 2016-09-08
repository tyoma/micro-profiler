#include "resource.h"

#include <atlbase.h>
#include <atlwin.h>
#include <charting/win32/display.h>
#include <charting/line.h>

using namespace std;

class random_model : public charting::line_xy::model
{
public:
	random_model()
		: _samples(100)
	{
		for (int i = 0; i != 100; ++i)
			update(i);
	}

	void update(int i = -1)
	{
		if (i == -1)
			i = static_cast<int>(_samples.size() * rand() / RAND_MAX);
		_samples[i].x = 1500.0f * rand() / RAND_MAX;
		_samples[i].y = 1000.0f * rand() / RAND_MAX;
		invalidated(0);
	}

private:
	virtual size_t get_count() const
	{
		return _samples.size();
	}

	virtual sample get_sample(size_t i) const
	{
		return _samples[i];
	}

private:
	vector<sample> _samples;
};

class MainDlg : public ATL::CDialogImpl<MainDlg>
{
public:
	enum {	IDD = IDD_MAIN	};

private:
	BEGIN_MSG_MAP(MainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog);
		MESSAGE_HANDLER(WM_CLOSE, OnClose);
		MESSAGE_HANDLER(WM_TIMER, OnTimer);
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		_model.reset(new random_model);
		_display = charting::create_display(m_hWnd);
		_display->add_series(charting::series_ptr(new charting::line_xy(_model)));
		SetTimer(1, 50);
		handled = TRUE;
		return 0;
	}

	LRESULT OnClose(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		EndDialog(0);
		handled = TRUE;
		return 0;
	}

	LRESULT OnTimer(UINT /*message*/, WPARAM /*wparam*/, LPARAM /*lparam*/, BOOL &handled)
	{
		_model->update();
		handled = TRUE;
		return 0;
	}

	virtual void OnFinalMessage(HWND hwnd)
	{
		CDialogImpl<MainDlg>::OnFinalMessage(hwnd);
	}

private:
	shared_ptr<charting::display> _display;
	shared_ptr<random_model> _model;
};

int main()
{
	MainDlg dlg;

	dlg.DoModal(NULL);
}
