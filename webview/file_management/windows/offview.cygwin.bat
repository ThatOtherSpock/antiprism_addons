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
start /min C:\cygwin64\bin\bash --login -i -c "offview.py '%filename%'"

GOTO End

:Syntax
ECHO.
ECHO command: %0
ECHO Version: 2.4
ECHO Written by Roger Kaufman (polyhedrasmith@gmail.com)
ECHO.
ECHO Send OFF file to offview.py via cygwin64
ECHO.
ECHO Usage:  offview off_file
ECHO Where:  off_file is any valid OFF file
ECHO.

:End
ENDLOCAL
