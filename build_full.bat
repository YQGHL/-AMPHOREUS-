@echo off
echo Building full LLM test client...
echo.

chcp 65001 >nul

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

cl.exe /std:c++20 /EHsc /O2 /utf-8 llm_test_full.cpp /Fe:llm_test_full.exe /link winhttp.lib

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Executable: llm_test_full.exe
    echo.
    echo Usage examples:
    echo   llm_test_full.exe
    echo   llm_test_full.exe --real
    echo   llm_test_full.exe --api http://localhost:8000 --model custom-model
    echo   llm_test_full.exe --help
) else (
    echo.
    echo Build failed!
)

echo.
pause