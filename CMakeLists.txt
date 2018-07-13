cmake_minimum_required (VERSION 3.11)
project(jtag_analyzer)

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
