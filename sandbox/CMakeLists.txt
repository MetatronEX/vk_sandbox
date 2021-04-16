cmake_minimum_required(VERSION 3.15)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME})

file(GLOB_RECURSE HEADERS 
  include/*.h  
  include/*.hpp
) 

file(GLOB_RECURSE SOURCES 
  src/*.cpp   
) 

# generate visual studio project filters
if (MSVC)
foreach(_source IN ITEMS ${HEADERS} ${SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
  $<$<CXX_COMPILER_ID:MSVC>:/MP>
  $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/include)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${VK_INC_DIR}
  ${GLM_INC_DIR}
  ${ENTT_INC_DIR}
  ${GLFW_INC_DIR}
  ${INCLUDE_DIR}
)

# link lib

target_link_libraries(${PROJECT_NAME} ${GLFW_LIB_DIR}/glfw3.lib)
target_link_libraries(${PROJECT_NAME} ${VK_LIB_DIR}/vulkan-1.lib)

