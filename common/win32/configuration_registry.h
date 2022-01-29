//	Copyright (c) 2011-2022 by Artem A. Gevorkyan (gevorkyan.org)
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

#include "../configuration.h"

struct HKEY__;
typedef struct HKEY__ *HKEY;

namespace micro_profiler
{
	class registry_hive : public hive
	{
	public:
		registry_hive(std::shared_ptr<void> key);

		static std::shared_ptr<hive> open_user_settings(const char *registry_path);

	private:
		// hive methods
		virtual std::shared_ptr<hive> create(const char *name) override;
		virtual std::shared_ptr<const hive> open(const char *name) const override;
		virtual void store(const char *name, int value) override;
		virtual void store(const char *name, const char *value) override;
		virtual bool load(const char *name, int &value) const override;
		virtual bool load(const char *name, std::string &value) const override;

		operator HKEY() const;

	private:
		std::shared_ptr<void> _key;
	};
}
