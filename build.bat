@echo off
setlocal
set project_root=%~dp0
set exe_name=kcpp
pushd %project_root%
if not exist build (
	mkdir build
)
rem --- Create a text tree of the code so we can skip the build if nothing 
rem     changed ---
pushd %project_root%\code
FOR /F "delims=" %%G IN ('DIR /B /S') DO (
	>>"%project_root%\build\code-tree-current.txt" ECHO %%~G,%%~tG,%%~zG
)
rem pop from %project_root%\code
popd
pushd build
rem --- Detect if the code tree differs, and if it doesn't, skip building ---
set codeTreeIsDifferent=FALSE
fc code-tree-existing.txt code-tree-current.txt > NUL 2> NUL
if %ERRORLEVEL% GTR 0 (
	set codeTreeIsDifferent=TRUE
)
del code-tree-existing.txt
ren code-tree-current.txt code-tree-existing.txt
IF "%codeTreeIsDifferent%"=="TRUE" (
	echo %exe_name% code tree has changed!  Continuing build...
) ELSE (
	echo %exe_name% code tree is unchanged!  Skipping build...
	GOTO :SKIP_BUILD
)
rem --- Build the executable ---
cl %project_root%\code\main.cpp /Fe%exe_name% /nologo /std:c++latest ^
	/Od /Oi /GR- /EHsc /Zi /FC /link /incremental:no 
:SKIP_BUILD
rem pop from build
popd
rem pop from %project_root%
popd
endlocal
exit /b 0