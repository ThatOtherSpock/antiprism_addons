@ECHO OFF
:: Use local environment
SETLOCAL

:: Check if a file is specified
IF [%1]==[] GOTO Syntax:Syntax

set filename=%*

:: https://stackoverflow.com/questions/23542453/change-backslash-to-forward-slash-in-windows-batch-file
:: https://stackoverflow.com/questions/16107246/how-to-add-quotes-to-string-in-a-batch-script
set filename=^"%filename:\=/%^"

:: https://stackoverflow.com/questions/43270097/creating-batch-file-to-start-cygwin-and-execute-specific-command
:: https://stackoverflow.com/questions/23057448/open-program-minimized-via-command-prompt
start /min C:\cygwin64\bin\bash --login -i -c "x3dview.py -v '%filename%'"

GOTO End

:Syntax
ECHO.
ECHO command: %0
ECHO Version: 3.0
ECHO Written by Roger Kaufman (polyhedrasmith@gmail.com)
ECHO.
ECHO Send VRML/X3D file to x3dview.py via cygwin64
ECHO.
ECHO Usage:  x3dview x3d_file
ECHO Where:  x3d_file is any valid X3D or VRML file (may be compressed)
ECHO.

:End
ENDLOCAL
