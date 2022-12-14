@ECHO OFF
set SOLNAME=asdfq-next

mkdist clean

PUSHD ..\vcproj

CALL :cleanprojdir edit

POPD
PUSHD ..
CALL :cleanprojdir bin
CALL :cleanroot %1
POPD
EXIT /B 0

:cleanprojdir
SETLOCAL
ECHO Cleaning %1
PUSHD %1
CALL :delstarifexist *.user
IF EXIST win32 rd /q /s win32
IF EXIST x64 rd /q /s x64
POPD
ENDLOCAL
EXIT /B

:delstarifexist
SETLOCAL
FOR %%F IN ("%1") DO (
	DEL /q "%%~nxF"
)
ENDLOCAL
EXIT /B

:cleanroot
SETLOCAL
ECHO Cleaning root dir...
IF EXIST ipch rd /q /s ipch
IF EXIST %SOLNAME%.sdf del /q %SOLNAME%.sdf
IF EXIST %SOLNAME%.suo (
	ATTRIB -h %SOLNAME%.suo
	DEL /q %SOLNAME%.suo
)

ENDLOCAL
EXIT /B