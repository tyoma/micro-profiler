var stdin = WScript.StdIn;
var stdout = WScript.StdOut;
var splitter = /(.*)\: (.*)/;
var fs = new ActiveXObject("Scripting.FileSystemObject");
var macros = {};

var macrosStream = fs.OpenTextFile(WScript.Arguments(0), 1);

while (!macrosStream.AtEndOfStream) {
	var line = macrosStream.ReadLine();
	var m = line.match(splitter);

	if (m)
		macros["\\\<" + m[1] + "\\\>"] = m[2];
}

while (!stdin.AtEndOfStream) {
	var line = stdin.ReadLine();

	for (var macro in macros)	{
		var rg = new RegExp(macro, "ig");

		line = line.replace(rg, macros[macro]);
	}
	stdout.WriteLine(line);
}
