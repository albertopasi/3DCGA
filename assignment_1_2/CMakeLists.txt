cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ComputerGraphics C CXX)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
elseif(PROJECT_IS_TOP_LEVEL)
	add_subdirectory("../../../framework/" "${CMAKE_CURRENT_BINARY_DIR}/framework/")
endif()

# Additional source files
add_library(ParticleSimLib "")
enable_sanitizers(ParticleSimLib)
set_project_warnings(ParticleSimLib)
include(${CMAKE_CURRENT_LIST_DIR}/src/CMakeLists.txt)
target_include_directories(ParticleSimLib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/")
target_compile_features(ParticleSimLib PUBLIC cxx_std_20)
target_link_libraries(ParticleSimLib PUBLIC CGFramework)

# Preprocessor definitions for paths
target_compile_definitions(
	ParticleSimLib
	PUBLIC
	"-DRESOURCES_DIR=\"${CMAKE_CURRENT_LIST_DIR}/resources/\""
	"-DSHADERS_DIR=\"${CMAKE_CURRENT_LIST_DIR}/shaders/\"")

# Main executable config
add_executable(Master_Practical_ParticleSimulation "src/main.cpp")
target_compile_features(Master_Practical_ParticleSimulation PUBLIC cxx_std_20)
target_link_libraries(Master_Practical_ParticleSimulation PRIVATE ParticleSimLib)
enable_sanitizers(Master_Practical_ParticleSimulation)
set_project_warnings(Master_Practical_ParticleSimulation)
