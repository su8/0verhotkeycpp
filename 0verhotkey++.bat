:TOP
@CLS
@ECHO OFF
TITLE kurger_cli
:BEGIN
if not exist C:\MingW\bin\0verhotkey++.exe (
  COPY kurger_cli.exe C:\MingW\bin\0verhotkey++.exe
)
CLS
:loop
C:\MingW\bin\0verhotkey++.exe
PAUSE
GOTO loop