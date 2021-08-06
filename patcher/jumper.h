//	Copyright (c) 2011-2021 by Artem A. Gevorkyan (gevorkyan.org)
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.

#pragma once

#include <common/platform.h>
#include <common/types.h>

namespace micro_profiler
{
	class jumper
	{
	public:
		jumper(void *target, const void *divert_to);
		~jumper();

		bool active() const;
		const void *entry() const;
		const void *target() const;
		bool activate(bool atomic);
		bool revert(bool atomic);
		void detach();

	private:
		byte *prologue() const;
		byte prologue_size() const;

	private:
		byte *_target;
		byte _fuse_revert[8], _fill;
		int _entry : 6, _active : 1, _detached : 1;
	};



	inline bool jumper::active() const
	{	return !_detached && !!_active;	}

	inline const void *jumper::entry() const
	{	return _target + _entry;	}

	inline const void *jumper::target() const
	{	return _target;	}
}
