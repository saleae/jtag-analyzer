cmake_minimum_required (VERSION 3.11)
project(jtag_analyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/JtagAnalyzer.cpp
src/JtagAnalyzer.h
src/JtagAnalyzerResults.cpp
src/JtagAnalyzerResults.h
src/JtagAnalyzerSettings.cpp
src/JtagAnalyzerSettings.h
src/JtagSimulationDataGenerator.cpp
src/JtagSimulationDataGenerator.h
src/JtagTypes.cpp
src/JtagTypes.h
)

add_analyzer_plugin(jtag_analyzer SOURCES ${SOURCES})
