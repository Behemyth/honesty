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

if(CMAKE_CONFIGURATION_TYPES)
	set(_test_is_multi_config ON)
else()
	set(_test_is_multi_config OFF)
endif()
set(_test_dependency_build_type "$<IF:$<STREQUAL:$<CONFIG>,Development>,Debug,$<CONFIG>>")

add_test(
	NAME test_package_consumer
	COMMAND ${CMAKE_COMMAND}
		"-DTEST_PROJECT_BINARY_DIR=${CMAKE_BINARY_DIR}"
		"-DTEST_SOURCE_DIR=${CMAKE_SOURCE_DIR}/test_package"
		"-DTEST_GENERATORS_DIR=${CMAKE_SOURCE_DIR}/build/conan/${_test_dependency_build_type}/generators"
		"-DTEST_GENERATOR=${CMAKE_GENERATOR}"
		"-DTEST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
		"-DTEST_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
		"-DTEST_CONFIG=$<CONFIG>"
		"-DTEST_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
		"-DTEST_IS_MULTI_CONFIG=${_test_is_multi_config}"
		"-DTEST_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
		"-DTEST_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
		-P "${CMAKE_CURRENT_LIST_DIR}/test_package.cmake"
)
