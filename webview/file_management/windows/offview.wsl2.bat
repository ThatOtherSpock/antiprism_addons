@ECHO OFF
:: Use local environment
SETLOCAL

:: Check if a file is specified
IF [%1]==[] GOTO Syntax:Syntax

set filename=%*

:: reformat filename for drive letter when called from explorer. filename will be quoted.

:: https://stackoverflow.com/questions/4983508/can-i-have-an-if-block-in-dos-batch-file
:: colon doesn't need escaping
if x%filename::=%==x%filename% goto :no_colon

:: filename contains a colon. get drive letter.
set letter=%filename:~1,1%
:: get the rest of the string after colon
set rest=%filename:~3%

:: convert to lower case
:: https://stackoverflow.com/questions/62026148/make-variable-all-lower-case-in-batch-without-call
set locase=for /L %%n in (1 1 2) do if %%n==2 ( for %%# in (a b c d e f g h i j k l m n o p q r s t u v w x y z) do set "result=!result:%%#=%%#!") ELSE setlocal enableDelayedExpansion ^& set result=
%locase%%letter%
set letter=%result%

:: change filename compatable with WSL. need starting quote
set filename="/mnt/%letter%%rest%

:no_colon

:: https://stackoverflow.com/questions/23542453/change-backslash-to-forward-slash-in-windows-batch-file
:: https://stackoverflow.com/questions/16107246/how-to-add-quotes-to-string-in-a-batch-script
set filename=^"%filename:\=/%^"

:: https://stackoverflow.com/questions/43270097/creating-batch-file-to-start-cygwin-and-execute-specific-command
:: https://stackoverflow.com/questions/23057448/open-program-minimized-via-command-prompt
start /min wsl bash --login -i -c "offview.py '%filename%'"

GOTO End

:Syntax
ECHO.
ECHO command: %0
ECHO Version: 3.0
ECHO Written by Roger Kaufman (polyhedrasmith@gmail.com)
ECHO.
ECHO Send OFF file to offview.py via WSL2
ECHO.
ECHO Usage:  offview off_file
ECHO Where:  off_file is any valid OFF file
ECHO.

:End
ENDLOCAL
