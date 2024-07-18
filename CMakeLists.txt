cmake_minimum_required(VERSION 3.1)
project (simplesquirrel)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)

# Some required properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Select build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Grab the files
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/simplesquirrel/*.hpp)

include_directories(${INCLUDE_DIRECTORIES} ${SQUIRREL_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
link_directories(${CMAKE_LIBRARY_PATH})

# Add our library
add_library(${PROJECT_NAME}_static STATIC ${SOURCES} ${HEADERS})
add_dependencies(${PROJECT_NAME}_static squirrel_static sqstdlib_static)
target_link_libraries(${PROJECT_NAME}_static squirrel_static sqstdlib_static)
target_compile_definitions(${PROJECT_NAME}_static PRIVATE SSQ_EXPORTS=1 SSQ_DLL=1)
