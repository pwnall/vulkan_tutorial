cmake_minimum_required(VERSION 3.13)
project(VulkanTutorial VERSION 1.1.0 LANGUAGES CXX)

# This project requires C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# https://github.com/izenecloud/cmake/blob/master/SetCompilerWarningAll.cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use the highest warning level for Visual Studio.
  set(CMAKE_CXX_WARNING_LEVEL 4)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")

  # Disable C++ exceptions.
  string(REGEX REPLACE "/EH[a-z]+" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHs-c-")
  add_definitions(-D_HAS_EXCEPTIONS=0)

  # Disable RTTI.
  string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
else(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # Use -Wall for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wall")

  # Use -Wextra for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Wextra")

  # Use -Werror for clang and gcc.
  if(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  endif(NOT CMAKE_CXX_FLAGS MATCHES "-Werror")

  # Disable C++ exceptions.
  string(REGEX REPLACE "-fexceptions" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")

  # Disable RTTI.
  string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)
find_program(glslc_binary NAMES glslc HINT Vulkan::glslc REQUIRED)

add_custom_target(spirv_shaders ALL)
function(spirv_shader glsl_source spirv_module)
  add_custom_command(
    OUTPUT
      "${spirv_module}"
    COMMAND
      "${glslc_binary}"
      ARGS
        "-o"
        "${CMAKE_CURRENT_BINARY_DIR}/${spirv_module}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${glsl_source}"
    MAIN_DEPENDENCY
      "${glsl_source}"
    COMMENT
      "Building SPIR-V module ${spirv_module}"
    VERBATIM
  )

  target_sources(spirv_shaders PRIVATE "${spirv_module}")
endfunction(spirv_shader)

spirv_shader(shaders/shader.vert vert.spv)
spirv_shader(shaders/shader.frag frag.spv)

add_library(gl_deps INTERFACE)
target_link_libraries(gl_deps
  INTERFACE
    glfw
    glm::glm
    Vulkan::Vulkan
)
target_compile_definitions(gl_deps
  INTERFACE
    VULKAN_HPP_NO_EXCEPTIONS)

add_executable(development_environment development_environment.cc)
target_link_libraries(development_environment PRIVATE gl_deps)

add_library(triangle_library "")
target_sources(triangle_library
  PRIVATE
    "vulkan_config.cc"
    "vulkan_device.cc"
    "vulkan_errors.cc"
    "vulkan_extension_list.cc"
    "vulkan_layer_list.cc"
    "vulkan_physical_device.cc"
    "vulkan_physical_device_list.cc"
    "vulkan_presentation_context.cc"
    "vulkan_surface_support.cc"
  PUBLIC
    "vulkan_config.h"
    "vulkan_device.h"
    "vulkan_errors.h"
    "vulkan_extension_list.h"
    "vulkan_layer_list.h"
    "vulkan_physical_device.h"
    "vulkan_physical_device_list.h"
    "vulkan_presentation_context.h"
    "vulkan_surface_support.h"
)
target_link_libraries(triangle_library
  PUBLIC
    gl_deps)

add_executable(hello_triangle "")
target_sources(hello_triangle
  PRIVATE
    hello_triangle.cc
)
target_link_libraries(hello_triangle
  PRIVATE
    gl_deps
    triangle_library
)

# glfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
