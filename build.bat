@echo off
if not exist build (
	mkdir build
)
set project_root=%~dp0
pushd build
cl %project_root%\code\main.cpp /Fekc++ /nologo /std:c++latest ^
	/Od /Oi /GR- /EHa- /Zi /FC /link /incremental:no 
popd