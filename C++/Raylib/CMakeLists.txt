cmake_minimum_required(VERSION 3.20)
project(LanguagePerformance CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(raylib REQUIRED)

file(GLOB SOURCES "src/*.cpp")

add_executable(LanguagePerformance ${SOURCES})

target_link_libraries(LanguagePerformance PRIVATE raylib)

if(WIN32)
    set(RC_FILE "${CMAKE_SOURCE_DIR}/src/icon.rc")
    if(EXISTS ${RC_FILE})
        target_sources(LanguagePerformance PRIVATE ${RC_FILE})
    endif()
endif()

add_custom_command(TARGET LanguagePerformance POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:LanguagePerformance>/data
)