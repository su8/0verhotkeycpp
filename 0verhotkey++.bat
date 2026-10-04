:TOP
@CLS
@ECHO OFF
TITLE 0verhotkey++
:BEGIN
if not exist C:\MingW\bin\0verhotkey++.exe (
  COPY 0verhotkey++.exe C:\MingW\bin\0verhotkey++.exe
)
CLS
:loop
C:\MingW\bin\0verhotkey++.exe
PAUSE
GOTO loop