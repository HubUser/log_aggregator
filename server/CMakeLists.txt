project (server)
cmake_minimum_required(VERSION 3.13)

set(BOOST_DIR /Users/vitaliishastun/InstalledApps/boost_binaries/include)
set(BOOST_LIB_DIR /Users/vitaliishastun/InstalledApps/boost_binaries/lib/)
set(BOOST_SYSTEM_LIB boost_system)
set(INCLUDE_DIR ../include)

set(example_sources main.cpp)

set_source_files_properties(${example_sources} PROPERTIES INCLUDE_DIRECTORIES ${BOOST_DIR})
set_source_files_properties(${example_sources} PROPERTIES COMPILE_OPTIONS "-std=c++17")

add_executable(server ${example_sources})
target_include_directories(server PUBLIC ${BOOST_DIR} ${INCLUDE_DIR})
target_link_directories(server PUBLIC ${BOOST_LIB_DIR})
target_link_libraries(server PUBLIC ${BOOST_SYSTEM_LIB})
