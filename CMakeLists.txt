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

add_library(jtag_analyzer SHARED ${SOURCES})
target_link_libraries(jtag_analyzer PRIVATE Saleae::AnalyzerSDK)

if(MSVC)
    install(TARGETS jtag_analyzer RUNTIME DESTINATION ".")
else()
    install(TARGETS jtag_analyzer LIBRARY DESTINATION ".")
endif()
