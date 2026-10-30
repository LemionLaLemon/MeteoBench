cmake_minimum_required(VERSION 3.21)
project(meteobench LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)
qt_standard_project_setup()

qt_add_executable(meteobench
    src/main.cpp
    src/wlogTableModel.cpp 
    src/qcustomplot.cpp
    src/getRecord.cpp
)

target_compile_options(meteobench PRIVATE -Wall -Wextra)
target_link_libraries(meteobench PRIVATE Qt6::Widgets Qt6::PrintSupport)
