cmake_minimum_required(VERSION 3.16)
project(RecoverPlan VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB RECOVERPLAN_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(recoverplan ${RECOVERPLAN_SOURCES})
target_include_directories(recoverplan PRIVATE include)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME cli_smoke
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/smoke_test.py
                         $<TARGET_FILE:recoverplan>)
    endif()
endif()
