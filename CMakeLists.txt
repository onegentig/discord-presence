cmake_minimum_required(VERSION 3.21)
project(discord-rpc)

set(CMAKE_CXX_STANDARD 23)
include(cmake/CPM.cmake)

add_library(${PROJECT_NAME} STATIC src/discord-rpc.cpp src/serialization.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

CPMAddPackage("gh:fmtlib/fmt#11.0.2")
CPMAddPackage("gh:stephenberry/glaze@4.4.3")
target_link_libraries(${PROJECT_NAME} PRIVATE fmt glaze::glaze)

if (DISCORD_RPC_BUILD_TESTS)
  add_subdirectory(test)
endif()