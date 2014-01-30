//	Copyright (c) 2011-2014 by Artem A. Gevorkyan (gevorkyan.org)
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

var stdin = WScript.StdIn;
var stdout = WScript.StdOut;
var splitter = /(.*)\: (.*)/;
var prefix = WScript.Arguments.length > 0 ? WScript.Arguments(0) : "";

while (!stdin.AtEndOfStream) {
	var line = stdin.ReadLine();
	var m = line.match(splitter);

	if (m) {
		var macro = prefix + "_" + m[1].toUpperCase().replace(/ /g, "_");
		var value = m[2].replace(/\\/g, "\\\\");
		var value_str = "\"" + value + "\"";

		stdout.WriteLine("#define " + macro + " " + value);
		stdout.WriteLine("#define " + macro + "_STR " + value_str);
	}
}
