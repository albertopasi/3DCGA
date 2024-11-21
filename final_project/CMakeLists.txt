cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ComputerGraphics C CXX)

# Set this before including framework such that it knows to use the OpenGL4.5 version of GLAD
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
elseif(PROJECT_IS_TOP_LEVEL)
	add_subdirectory("../../../framework/" "${CMAKE_CURRENT_BINARY_DIR}/framework/")
endif()

add_executable(Master_TechDemo
    "src/application.cpp"
    "src/texture.cpp"
	"src/mesh.cpp"
		src/Camera.cpp
		src/Camera.h
		src/Light.cpp
		src/Light.h
		src/utils.cpp
		src/utils.h
		src/curve.cpp
		src/curve.h
		src/point.cpp
		src/point.h
		src/robot.cpp
		src/robot.h
		src/terrain.cpp
		src/terrain.h
		src/particle_system.cpp
		src/particle_system.h
		src/minimap.h
		src/minimap.cpp
)

target_compile_definitions(Master_TechDemo PRIVATE RESOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}/")
target_compile_features(Master_TechDemo PRIVATE cxx_std_20)
target_link_libraries(Master_TechDemo PRIVATE CGFramework)
enable_sanitizers(Master_TechDemo)
set_project_warnings(Master_TechDemo)

# Copy all files in the resources folder to the build directory after every successful build.
add_custom_command(TARGET Master_TechDemo POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_LIST_DIR}/resources/" "$<TARGET_FILE_DIR:Master_TechDemo>/resources/")

# We would like to copy the files when they changed. Even if no *.cpp files were modified (and
# thus no build is triggered). We tell CMake that the executable depends on the shader files in
# the build directory. We also tell it how to generate those files (by copying them from the
# shaders folder in this directory). The gather all glsl files in the shaders folder when CMake
# is configured. So if you were to add a shader file then you need to configure CMake again.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/")

set(Master_TechDemo_shader_copies "")
# List all *.glsl files in the shaders folder
file(GLOB Master_TechDemo_shader_sources "${CMAKE_CURRENT_LIST_DIR}/shaders/*.glsl")
foreach (shader_file IN LISTS Master_TechDemo_shader_sources)
	get_filename_component(file_name ${shader_file} NAME)
	message("shader_file: ${file_name}")
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		DEPENDS "${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		)
	LIST(APPEND Master_TechDemo_shader_copies "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}")
endforeach()
add_custom_target(Master_TechDemo_copy_shaders DEPENDS ${Master_TechDemo_shader_copies})
add_dependencies(Master_TechDemo Master_TechDemo_copy_shaders)
