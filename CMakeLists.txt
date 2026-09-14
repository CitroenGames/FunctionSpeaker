cmake_minimum_required(VERSION 3.20)
project(FunctionSpeaker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to build examples (defaults to ON if top-level project)
option(BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

# FunctionSpeaker library
add_library(${PROJECT_NAME} STATIC
    src/FunctionSpeaker.h
    src/FunctionSpeaker.cpp
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# Compiler warning flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_EXAMPLES)
    add_executable(FunctionSpeakerExample
        example/main.cpp
    )
    target_link_libraries(FunctionSpeakerExample PRIVATE ${PROJECT_NAME})
    if(MSVC)
        target_compile_options(FunctionSpeakerExample PRIVATE /W4 /permissive-)
    else()
        target_compile_options(FunctionSpeakerExample PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()