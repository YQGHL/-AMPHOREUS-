$ErrorActionPreference = "Continue"
$env:INCLUDE = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\include;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um;C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\shared"
$env:LIB = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\um\x64;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x64"
$cl = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
$args = "/std:c++20", "/utf-8", "/EHsc", "/DNOMINMAX", "/Fe:AMPH0REUS.exe", "main.cpp", "BioAgent.cpp", "SimulationEnvironment.cpp", "LLMClient.cpp"
& $cl @args 2>&1 | Out-File -FilePath "cl_output.txt" -Encoding UTF8
Write-Host "编译完成，输出已保存到 cl_output.txt"