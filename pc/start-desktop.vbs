Option Explicit

Dim fs, shell, env, root, launcher, cmd
Set fs = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")
root = fs.GetParentFolderName(fs.GetParentFolderName(WScript.ScriptFullName))

launcher = fs.BuildPath(root, ".local\start-subscreen.cmd")
If Not fs.FileExists(launcher) Then
    launcher = fs.BuildPath(root, "pc\start-watch.cmd")
End If

shell.CurrentDirectory = root
Set env = shell.Environment("Process")
env("SUBSCREEN_DESKTOP_LAUNCH") = "1"
cmd = """" & shell.ExpandEnvironmentStrings("%ComSpec%") & """ /d /s /c """"" & launcher & """ --touch --once"""
shell.Run cmd, 0, False
