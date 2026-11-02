cmake_minimum_required(VERSION 4.5 FATAL_ERROR)

project(tests
	DESCRIPTION "Tests for the honesty project"
	LANGUAGES CXX
)

add_executable(tests)

set_target_properties(tests
	PROPERTIES
	LINKER_LANGUAGE CXX
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS NO
)

find_package(opentelemetry-cpp CONFIG REQUIRED)

target_link_libraries(tests
	PRIVATE
	opentelemetry-cpp::opentelemetry-cpp
	opentelemetry-cpp::logs
	opentelemetry-cpp::common
	opentelemetry-cpp::exporter_ostream_logs
)

target_link_libraries(tests
	PRIVATE
	synodic::honesty::main
)

target_sources(tests
	PUBLIC
	FILE_SET mock
	TYPE CXX_MODULES
)

add_subdirectory(
	mock
)

add_subdirectory(
	unit
)

# One CTest test per top-level Honesty test, discovered from 'tests list' when ctest runs
discover_tests(COMMAND tests
	DISCOVERY_ARGS list
	DISCOVERY_MATCH "^([^. ]+)\\.([^ ]+)$"
	TEST_NAME "\\1.\\2"
	TEST_ARGS --filter "\\1.\\2"
	BUILD_DEPENDS tests
)

# Glob all files to add a source group tree
file(GLOB_RECURSE file_list ${CMAKE_CURRENT_SOURCE_DIR}/*)

# Must be called from the target definition directory. Only matched against target source files (No nested CMakeList.txt, docs, etc).
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${file_list})

# Make the 'tests' project the default startup
set_property(
	DIRECTORY
	${CMAKE_SOURCE_DIR}
	PROPERTY
	VS_STARTUP_PROJECT tests
)
