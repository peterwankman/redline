@echo off

SET TMPFILE=revcount.txt
SET OUTFILE=src/rev.h

IF [%1] == [] (
	SET revision=HEAD
) ELSE (
	SET revision=%1
)

PUSHD ..
git rev-list --count %revision% > %TMPFILE%
SET /p REV=<%TMPFILE%
DEL /Q %TMPFILE%

ECHO /* THIS FILE HAS BEEN AUTOMATICALLY GENERATED */ > %OUTFILE%
ECHO #ifndef APP_VER_REV >> %OUTFILE%
IF [%REV%]==[] (
	ECHO #define APP_VER_REV 9999 >> %OUTFILE%
) ELSE (
	ECHO #define APP_VER_REV %REV% >> %OUTFILE%
)
ECHO #endif >> %OUTFILE%

POPD

EXIT /B 0