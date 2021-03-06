# CMake entry point
cmake_minimum_required (VERSION 3.1)
project (sculpt)

find_package(OpenGL REQUIRED)

# get rid of annoying MSVC warnings.
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# Compile external dependencies
add_subdirectory (deps)

set (CMAKE_CXX_STANDARD 11)


include_directories(
	deps/glfw-3.2/include/GLFW/
	deps/glm-0.9.7.5/
	deps/glfw-3.2/deps/
)


if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	-lGL -lGLU -lXrandr -lXext -lX11 -lrt
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(OPENGL_LIBRARY
	${OPENGL_LIBRARY}
	${CMAKE_DL_LIBS}
	${GLFW_LIBRARIES}
)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
)

add_executable(sculpture
  src/main.cpp
  src/gl_common.hpp
  src/shader.hpp
  src/marching_cubes.hpp
  src/marching_cubes_tables.hpp

  src/deform.cpp

  src/half_edge_mesh.cpp
  src/half_edge_mesh.hpp


  deps/glfw-3.2/deps/glad.c
	)

target_link_libraries(sculpture
	${ALL_LIBS}
)
