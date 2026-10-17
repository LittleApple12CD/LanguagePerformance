cmake_minimum_required(VERSION 3.20)
project(LanguagePerformance CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

file(GLOB SOURCES "src/*.cpp")

set(APP_RC "")
if(WIN32)
    set(APP_RC "${CMAKE_SOURCE_DIR}/src/icon.rc")
    if(MINGW)
        set_source_files_properties(${APP_RC} PROPERTIES LANGUAGE RC)
    endif()
endif()

add_executable(LanguagePerformance ${SOURCES} ${APP_RC})

target_link_libraries(LanguagePerformance PRIVATE SFML::Graphics)

add_custom_command(TARGET LanguagePerformance POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:LanguagePerformance>/data
)
