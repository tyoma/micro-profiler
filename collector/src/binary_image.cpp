//	Copyright (c) 2011-2018 by Artem A. Gevorkyan (gevorkyan.org)
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

#include <collector/binary_image.h>

#include <common/module.h>
#include <common/symbol_resolver.h>

using namespace std;
using namespace placeholders;

namespace micro_profiler
{
	class function_body_x86 : public function_body
	{
	public:
		function_body_x86(const vector<byte> &image, void *effective_base, const symbol_info &symbol);

	private:
		virtual string name() const;
		virtual void *effective_address() const;
		virtual const_byte_range body() const;

	private:
		string _name;
		byte *_effective_address;
		const_byte_range _body;
	};

	class binary_image_x86 : public binary_image
	{
	public:
		binary_image_x86(const wchar_t *image_path, void *effective_base);

	private:
		virtual void enumerate_functions(const function_callback &cb) const;

		void invoke_callback(const function_callback &callback, const symbol_info &symbol) const
		{	callback(function_body_x86(_image, _effective_base, symbol));	}

	private:
		shared_ptr<symbol_resolver> _resolver;
		void *_effective_base;
		vector<byte> _image;
	};



	function_body_x86::function_body_x86(const vector<byte> &image, void *effective_base,
			const symbol_info &symbol)
		: _name(symbol.name), _effective_address(static_cast<byte *>(symbol.location)),
			_body(&image[_effective_address - static_cast<const byte *>(effective_base) + 0x400 - 0x1000], symbol.size)
	{	}

	string function_body_x86::name() const
	{	return _name;	}

	void *function_body_x86::effective_address() const
	{	return _effective_address;	}

	const_byte_range function_body_x86::body() const
	{	return _body;	}

	binary_image_x86::binary_image_x86(const wchar_t *image_path, void *effective_base)
		: _resolver(symbol_resolver::create()), _effective_base(effective_base)
	{
		shared_ptr<FILE> file(_wfopen(image_path, L"rb"), &fclose);
		byte buffer[4096];
		unsigned read;

		do
		{
			read = fread(buffer, 1, sizeof(buffer), file.get());
			_image.insert(_image.end(), buffer, buffer + read);
		} while (read == sizeof(buffer));
		_resolver->add_image(image_path, (size_t)_effective_base);
	}

	void binary_image_x86::enumerate_functions(const function_callback &callback) const
	{	_resolver->enumerate_symbols((size_t)_effective_base, bind(&binary_image_x86::invoke_callback, this, callback, _1));	}


	shared_ptr<binary_image> load_image_at(void *base)
	{
		module_info mi = get_module_info(base);

		return shared_ptr<binary_image>(new binary_image_x86(mi.path.c_str(), base));
	}
}
