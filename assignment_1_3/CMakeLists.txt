cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(ComputerGraphics C CXX)


if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
elseif(PROJECT_IS_TOP_LEVEL)
	add_subdirectory("../../../framework/" "${CMAKE_CURRENT_BINARY_DIR}/framework/")
endif()

add_executable(Master_Practical_DiffusionCurves
	"src/main.cpp"
	"src/shapes.h"
	"src/shapes.cpp"
	"src/rapidxml.hpp"
	"src/rapidxml_utils.hpp"
	)
target_compile_features(Master_Practical_DiffusionCurves PRIVATE cxx_std_20)
target_compile_definitions(Master_Practical_DiffusionCurves PRIVATE RESOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}/")

# Link to OpenGL, and Microsoft-GSL and/or make their header files available.
target_link_libraries(Master_Practical_DiffusionCurves PUBLIC glm)
target_link_libraries(Master_Practical_DiffusionCurves PRIVATE CGFramework)
enable_sanitizers(Master_Practical_DiffusionCurves)
set_project_warnings(Master_Practical_DiffusionCurves)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")
# Copy all files in the resources folder to the build directory after every successful build.
add_custom_command(TARGET Master_Practical_DiffusionCurves POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_LIST_DIR}/resources/" "${CMAKE_CURRENT_BINARY_DIR}/resources/")


# We would like to copy the files when they changed. Even if no *.cpp files were modified (and
# thus no build is triggered). We tell CMake that the executable depends on the shader files in
# the build directory. We also tell it how to generate those files (by copying them from the
# shaders folder in this directory). The gather all glsl files in the shaders folder when CMake
# is configured. So if you were to add a shader file then you need to configure CMake again.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/")


set(Master_Practical_DiffusionCurves_shader_copies "")
file(GLOB Master_Practical_DiffusionCurves_shader_sources "${CMAKE_CURRENT_LIST_DIR}/shaders/*.glsl")
message("${Master_Practical_DiffusionCurves_shader_sources}")
foreach (shader_file IN LISTS Master_Practical_DiffusionCurves_shader_sources)
	message("${shader_file}")
	get_filename_component(file_name ${shader_file} NAME)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		DEPENDS "${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		)
	LIST(APPEND Master_Practical_DiffusionCurves_shader_copies "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}")
endforeach()
message("${Master_Practical_DiffusionCurves_shader_copies}")
add_custom_target(Master_Practical_DiffusionCurves_copy_shaders DEPENDS ${Master_Practical_DiffusionCurves_shader_copies})
add_dependencies(Master_Practical_DiffusionCurves Master_Practical_DiffusionCurves_copy_shaders)