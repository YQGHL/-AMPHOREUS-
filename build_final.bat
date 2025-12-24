@echo off
echo Building LLM test program...
echo.

REM Set code page to UTF-8
chcp 65001 >nul

REM Setup Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

REM Compile with UTF-8 support
cl.exe /std:c++20 /EHsc /O2 /utf-8 llm_test_utf8.cpp /Fe:llm_test.exe

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Executable: llm_test.exe
    echo.
    echo To run: llm_test.exe
) else (
    echo.
    echo Build failed!
)

echo.
pause