add_library(libMemoryKV libMemoryKV.c)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.17.0)

# link math library (libm)
target_link_libraries(${PROJECT_NAME} PUBLIC m)
target_link_libraries(${PROJECT_NAME} PUBLIC PkgConfig::libcurl)
