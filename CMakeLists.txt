cmake_minimum_required(VERSION 4.1)
project(AMPH0REUS)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# 添加可执行文件 - AMPH0REUS主项目
add_executable(AMPH0REUS 
    main.cpp 
    BioAgent.cpp 
    SimulationEnvironment.cpp 
    LLMClient.cpp
        main.cpp
        LLMClient.cpp
        main.cpp
        main.cpp
        main.cpp
)

# Windows特定设置
if(WIN32)
    # 链接库 - LLMClient需要winhttp和shell32 (所有Windows编译器都需要)
    target_link_libraries(AMPH0REUS PRIVATE winhttp shell32)
    
    # 编译器特定选项
    if(MSVC)
        # Visual Studio 编译器选项
        target_compile_options(AMPH0REUS PRIVATE /EHsc /DNOMINMAX /utf-8)
        # 设置子系统为控制台 (MSVC)
        set_target_properties(AMPH0REUS PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
    elseif(MINGW)
        # MinGW (GCC) 编译器选项
        target_compile_options(AMPH0REUS PRIVATE -m32 -O2 -DNOMINMAX -fexec-charset=UTF-8 -finput-charset=UTF-8 -fexceptions)
        # 设置控制台子系统 (MinGW) 并传递 -m32 给链接器
        set_target_properties(AMPH0REUS PROPERTIES LINK_FLAGS "-m32 -mconsole")
    else()
        # 其他Windows编译器 (Clang等)
        target_compile_options(AMPH0REUS PRIVATE -O2 -DNOMINMAX)
        message(WARNING "Unknown Windows compiler, using generic flags")
    endif()
endif()

# 设置可执行文件属性
set_target_properties(AMPH0REUS PROPERTIES
    OUTPUT_NAME "AMPH0REUS"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)