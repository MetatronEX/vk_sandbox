cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(vk_sandbox)

set(VK_ROOT $ENV{VULKAN_SDK})

set(GLM_ROOT ${CMAKE_SOURCE_DIR}/external/glm)
set(SDL_ROOT $(CMAKE_SOURCE_DIR}/external/SDL)

set(GLFW_ROOT ${CMAKE_SOURCE_DIR}/external/glfw)
set(ENTT_ROOT ${CMAKE_SOURCE_DIR}/external/entt)

set(VK_INC_DIR ${VK_ROOT}/Include)
set(VK_LIB_DIR ${VK_ROOT}/Lib)

set(GLM_INC_DIR ${GLM_ROOT}/glm)
set(SDL_INC_DIR ${SDL_ROOT}/include)

set(ENTT_INC_DIR ${ENTT_ROOT}/single_include)
set(GLFW_INC_DIR ${GLFW_ROOT}/include)