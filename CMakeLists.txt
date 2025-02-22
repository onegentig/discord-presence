cmake_minimum_required(VERSION 3.21)

project(discord-rpc)

set(CMAKE_CXX_STANDARD 23)
include(cmake/CPM.cmake)

add_library(${PROJECT_NAME} STATIC src/discord-rpc.cpp src/serialization.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

# Windows has some extra code
if (WIN32)
  target_sources(${PROJECT_NAME} PRIVATE src/platform/windows.cpp)

  if (CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(TARGET_MASM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/wine_masm.asm")
    set(TARGET_OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/wine_masm.obj")
    add_custom_command(
      OUTPUT "${TARGET_OBJ_FILE}"
      COMMAND ml64 /c /Fo${TARGET_OBJ_FILE} "${TARGET_MASM_FILE}"
      DEPENDS "${TARGET_MASM_FILE}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Assembling ${TARGET_MASM_FILE}"
      VERBATIM
    )

    add_custom_target(assemble_wine_masm ALL DEPENDS "${TARGET_OBJ_FILE}")
    add_dependencies(${PROJECT_NAME} assemble_wine_masm)

    target_link_libraries(${PROJECT_NAME} PRIVATE "${TARGET_OBJ_FILE}")
  endif()
endif()

CPMAddPackage("gh:fmtlib/fmt#11.0.2")
CPMAddPackage("gh:stephenberry/glaze@4.4.3")
target_link_libraries(${PROJECT_NAME} PRIVATE fmt glaze::glaze)

if (DISCORD_RPC_BUILD_TESTS)
  add_subdirectory(test)
endif()