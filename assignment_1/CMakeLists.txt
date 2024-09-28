cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(ComputerGraphics C CXX)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/framework")
	# Create framework library and include CMake scripts (compiler warnings, sanitizers and static analyzers).
	add_subdirectory("framework")
elseif(PROJECT_IS_TOP_LEVEL)
	add_subdirectory("../../../framework/" "${CMAKE_CURRENT_BINARY_DIR}/framework/")
endif()

add_executable(Master_Assignment1_P1 "src/main.cpp"
)
target_compile_definitions(Master_Assignment1_P1 PRIVATE RESOURCE_ROOT="${CMAKE_CURRENT_LIST_DIR}/")
target_compile_features(Master_Assignment1_P1 PRIVATE cxx_std_20)
target_link_libraries(Master_Assignment1_P1 PRIVATE CGFramework)
enable_sanitizers(Master_Assignment1_P1)
set_project_warnings(Master_Assignment1_P1)

# Copy all files in the resources folder to the build directory after every successful build.
add_custom_command(TARGET Master_Assignment1_P1 POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	"${CMAKE_CURRENT_LIST_DIR}/resources/" "$<TARGET_FILE_DIR:Master_Assignment1_P1>/resources/")

# We would like to copy the files when they changed. Even if no *.cpp files were modified (and
# thus no build is triggered). We tell CMake that the executable depends on the shader files in
# the build directory. We also tell it how to generate those files (by copying them from the
# shaders folder in this directory). The gather all glsl files in the shaders folder when CMake
# is configured. So if you were to add a shader file then you need to configure CMake again.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders/")

set(Master_Assignment1_P1_shader_copies "")
# List all *.glsl files in the shaders folder
file(GLOB Master_Assignment1_P1_shader_sources "${CMAKE_CURRENT_LIST_DIR}/shaders/*.glsl")


foreach (shader_file IN LISTS Master_Assignment1_P1_shader_sources)
	# Tell CMake that we need the files in the binary binary folder and tell it
	# how to achieve that (by copying the files). This way, CMake will look for
	# changes in the source files and will copy them automatically.
	get_filename_component(file_name ${shader_file} NAME)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		"${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}"
		DEPENDS "${CMAKE_CURRENT_LIST_DIR}/shaders/${file_name}"
		)
	LIST(APPEND Master_Assignment1_P1_shader_copies "${CMAKE_CURRENT_BINARY_DIR}/shaders/${file_name}")
endforeach()

add_custom_target(Master_Assignment1_P1_copy_shaders DEPENDS ${Master_Assignment1_P1_shader_copies})
add_dependencies(Master_Assignment1_P1 Master_Assignment1_P1_copy_shaders)
