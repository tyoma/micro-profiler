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
