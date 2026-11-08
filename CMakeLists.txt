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
    src/stats.cpp
    src/drawPlotsTab.cpp
    src/drawDataTableTab.cpp
    src/settings.cpp
    src/validate.cpp
    src/validation.cpp
)

# Embed icons and stylesheet in the executable (accessed as :/icons/..., :/style.qss).
qt_add_resources(meteobench "assets"
    PREFIX "/"
    BASE src
    FILES
        src/style.qss
        src/icons/AutofitColumns.png
        src/icons/Clock.png
        src/icons/JumpToNextFile.png
        src/icons/JumpToPreviousFile.png
        src/icons/NormalizeColumns.png
        src/icons/Open.png
        src/icons/ResetLayout.png
        src/icons/Validate.png
)

target_compile_options(meteobench PRIVATE -Wall -Wextra)
target_link_libraries(meteobench PRIVATE Qt6::Widgets Qt6::PrintSupport)

# GUI app: no console window on Windows.
set_target_properties(meteobench PROPERTIES WIN32_EXECUTABLE TRUE)

if(APPLE)
    # Provides __isPlatformVersionAtLeast when cross-compiling without Darwin compiler-rt.
    target_sources(meteobench PRIVATE src/macosAvailability.cpp)
endif()
