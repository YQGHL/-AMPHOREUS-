call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
echo 正在编译 AMPH0REUS 项目...
cl.exe /std:c++20 /utf-8 /EHsc /Fe:AMPH0REUS.exe main.cpp BioAgent.cpp SimulationEnvironment.cpp LLMClient.cpp
if %errorlevel% neq 0 (
    echo 编译失败，错误代码: %errorlevel%
    exit /b %errorlevel%
)
echo 编译成功！