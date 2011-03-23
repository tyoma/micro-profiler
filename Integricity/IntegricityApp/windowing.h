#pragma once

#include "basics.h"

#include <windows.h>
#include <memory>
#include <functional>

class window_wrapper : public std::enable_shared_from_this<window_wrapper>
{
public:
	typedef std::function<LRESULT (UINT, WPARAM, LPARAM)> previous_handler_t;
	typedef std::function<LRESULT (UINT, WPARAM, LPARAM, const previous_handler_t &)> user_handler_t;

public:
	static std::shared_ptr<window_wrapper> attach(HWND hwnd);
	bool detach();

	std::shared_ptr<destructible> advise(const user_handler_t &user_handler);
	void unadvise() throw();

	HWND hwnd() const;

private:
	HWND _window;
	WNDPROC _previous_handler;
	std::shared_ptr<window_wrapper> _this;
	std::shared_ptr<user_handler_t> _user_handler;

	window_wrapper(HWND hwnd);

	static LRESULT CALLBACK windowproc_proxy(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static std::shared_ptr<window_wrapper> extract(HWND hwnd);
};
