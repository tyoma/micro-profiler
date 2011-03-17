#pragma once

#include <windows.h>
#include <memory>

class window_wrapper : public std::enable_shared_from_this<window_wrapper>
{
	HWND _window;
	WNDPROC _previous;
	std::shared_ptr<window_wrapper> _this;

	window_wrapper(HWND hwnd);

	static LRESULT CALLBACK windowproc_proxy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	static std::shared_ptr<window_wrapper> attach(HWND hwnd);

	HWND hwnd() const;
};
