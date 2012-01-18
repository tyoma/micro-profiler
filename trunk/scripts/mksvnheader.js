var stdin = WScript.StdIn;
var stdout = WScript.StdOut;
var splitter = /(.*)\: (.*)/;

while (!stdin.AtEndOfStream)	{
	var line = stdin.ReadLine();
	var m = line.match(splitter);

	if (m) {
		var macro = "SVN_" + m[1].toUpperCase().replace(/ /g, "_");
		var value = macro != "SVN_REVISION" && macro != "SVN_LAST_CHANGED_REV" ? "\"" + m[2] + "\"" : m[2];

		stdout.WriteLine("#define " + macro + " " + value);
	}
}
