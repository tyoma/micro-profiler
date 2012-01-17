Option Explicit

Const msiOpenDatabaseModeReadOnly = 0
Const msiOpenDatabaseModeTransact = 1

Dim argNum, argCount: argCount = Wscript.Arguments.Count
Dim installer : Set installer = Wscript.CreateObject("WindowsInstaller.Installer")

' Open database
Dim database : Set database = installer.OpenDatabase(Wscript.Arguments(0), msiOpenDatabaseModeTransact)

' Process SQL statements
For argNum = 1 To argCount - 1
	Dim query: query = Wscript.Arguments(argNum)
	Dim view: Set view = database.OpenView(query)
	
	view.Execute
Next
database.Commit
