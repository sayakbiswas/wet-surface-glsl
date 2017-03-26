# CMake entry point
cmake_minimum_required (VERSION 2.6)
project (wet-surface)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

find_package (OpenGL REQUIRED)

# Compile external dependencies
add_subdirectory (external)

include_directories(
	external/AntTweakBar-1.16/include/
	external/glfw-3.1.2/include/GLFW/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/assimp-3.0.1270/include/
	external/bullet-2.81-rev2613/src/
	common/
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model/Suzanne.obj DESTINATION Model/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Model/Suzanne.mtl DESTINATION Model/)

# Wet Surface
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Standard.vs.glsl DESTINATION shaders/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Standard.fs.glsl DESTINATION shaders/)
add_executable(wet-surface
	wet_surface.cpp
	common/objloader.cpp
	common/vboindexer.cpp
	common/texture.cpp
	shaders/Standard.vs.glsl
	shaders/Standard.fs.glsl
)

target_link_libraries(wet-surface
	${ALL_LIBS}
)
