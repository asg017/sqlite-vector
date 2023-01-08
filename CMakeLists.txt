
cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(sqlite-vector VERSION 0.0.0)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Adding nlohmann_json for json parsing
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(vendor/json)

# vendor in SQLite amalgammation
include_directories(vendor/sqlite-snapshot-202212231449)
link_directories(BEFORE vendor/sqlite-snapshot-202212231449)

add_library(sqlite-vector SHARED src/extension.cpp src/vectors.cpp)
target_link_libraries(sqlite-vector sqlite3)
target_link_libraries(sqlite-vector nlohmann_json::nlohmann_json)

set_target_properties(sqlite-vector PROPERTIES PREFIX "")
set_target_properties(sqlite-vector PROPERTIES PREFIX "")
set_target_properties(sqlite-vector PROPERTIES OUTPUT_NAME "vector0")

target_include_directories(sqlite-vector PRIVATE include/)