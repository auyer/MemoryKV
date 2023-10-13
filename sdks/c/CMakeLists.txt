cmake_minimum_required(VERSION 3.27.6)

project(MemoryKV)

add_executable(${PROJECT_NAME} example.c)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.17.0)

add_subdirectory(libMemoryKV)

target_include_directories(${PROJECT_NAME} PUBLIC libMemoryKV/)
target_link_directories(${PROJECT_NAME} PUBLIC libMemoryKV/)

target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::libcurl libMemoryKV)
