cmake_minimum_required(VERSION 3.21)
project(ACPRFrameMeter
    VERSION 1.0.0
    DESCRIPTION "Frame meter mod for GGXXACPR"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})

set(CMAKE_BUILD_TYPE Debug)

add_library(framemeter SHARED
    source/source.cpp
    source/frameMeter/frameMeter.cpp
    source/hooks/hooks.cpp
    source/render/render.cpp
    source/render/text.cpp
    source/settings/modMenu.cpp
)
target_include_directories(framemeter PRIVATE
    include
    source
    third_party
)
set_target_properties(framemeter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}/FrameMeter
    PREFIX ""
)
target_compile_options(framemeter PRIVATE
    "-msse2"
)

target_link_libraries(framemeter
    d3d9
    d3dcompiler
)
target_link_options(framemeter PRIVATE
    -static
)

add_custom_command(
    OUTPUT ${BIN_DIR}/FrameMeter/config.json
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/source/config.json
        ${BIN_DIR}/FrameMeter/config.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/config.json
    COMMENT "Copying config"
)
add_custom_target(copy_config ALL
    DEPENDS ${BIN_DIR}/FrameMeter/config.json
)

add_custom_command(
    OUTPUT ${BIN_DIR}/FrameMeter/resources/font.bin
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/font.bin
        ${BIN_DIR}/FrameMeter/resources/font.bin
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resources/font.bin
    COMMENT "Copying font.bin"
)
add_custom_target(copy_font_bin ALL
    DEPENDS ${BIN_DIR}/FrameMeter/resources/font.bin
)
