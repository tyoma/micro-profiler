//	Copyright (C) 2011 by Artem A. Gevorkyan (gevorkyan.org)
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

var $updates = [
	"INSERT INTO Component (Component, ComponentId, Directory_, Attributes, Condition) VALUES ('c_env_usr', '{EAD8E58C-BC04-4509-A46C-7CA9C9190672}', 'TARGETDIR', 0, 'NOT ALLUSERS')",
	"INSERT INTO Component (Component, ComponentId, Directory_, Attributes, Condition) VALUES ('c_env_sys', '{C28237F2-C691-4E29-911D-4AA87E32AA7B}', 'TARGETDIR', 0, 'ALLUSERS')",
	"INSERT INTO FeatureComponents (Feature_, Component_) VALUES ('DefaultFeature', 'c_env_usr')",
	"INSERT INTO FeatureComponents (Feature_, Component_) VALUES ('DefaultFeature', 'c_env_sys')",
	"INSERT INTO Environment (Environment, Name, Value, Component_) VALUES ('env_mpdir_usr', 'MICROPROFILERDIR','[TARGETDIR]','c_env_usr')",
	"INSERT INTO Environment (Environment, Name, Value, Component_) VALUES ('env_path_usr', 'Path','[~];%MICROPROFILERDIR%','c_env_usr')",
	"INSERT INTO Environment (Environment, Name, Value, Component_) VALUES ('env_mpdir_sys', '*MICROPROFILERDIR','[TARGETDIR]','c_env_sys')",
	"INSERT INTO Environment (Environment, Name, Value, Component_) VALUES ('env_path_sys', '*Path','[~];%MICROPROFILERDIR%','c_env_sys')",
	"DELETE FROM Binary WHERE Name='DefBannerBitmap'"
];


var msiOpenDatabaseModeReadOnly = 0;
var msiOpenDatabaseModeTransact = 1;
var msiOpenDatabaseModeDirect = 2;

var installer = WScript.CreateObject("WindowsInstaller.Installer")

// Open database
var database = installer.OpenDatabase(WScript.Arguments(0), msiOpenDatabaseModeTransact);

// Process SQL statements
for (var i = 0; i != $updates.length; ++i)
	database.OpenView($updates[i]).Execute();

database.Commit();
