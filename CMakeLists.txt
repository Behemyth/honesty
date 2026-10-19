cmake_minimum_required(VERSION 4.0 FATAL_ERROR)

# Enable std module support for MSVC - MUST be before project()
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

set(CMAKE_CXX_MODULE_STD ON)

project(honesty
	VERSION 0.1.0
	DESCRIPTION "Honesty"
	LANGUAGES CXX
)

option(BUILD_TESTING "Build the tests verifying Honesty functionality" ON)

# Register our custom module location with CMake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/tool/cmake/modules")

# Provide the main library
add_library(honesty STATIC)
add_library(synodic::honesty ALIAS honesty)

# Provide a convenience variant with a main function
add_library(honesty_main STATIC)
add_library(synodic::honesty::main ALIAS honesty_main)

set_target_properties(honesty honesty_main
	PROPERTIES
	LINKER_LANGUAGE CXX
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS NO
	FOLDER "honesty"
)

target_compile_features(honesty PUBLIC cxx_std_23)
target_compile_features(honesty_main PUBLIC cxx_std_23)

find_package(opentelemetry-cpp REQUIRED)
find_package(periapsis REQUIRED)
find_package(portability REQUIRED)

target_link_libraries(honesty
	PUBLIC
	synodic::portability
	PRIVATE
	opentelemetry-cpp::opentelemetry-cpp
	synodic::periapsis
)

# Link the main library to the convenience variant
target_link_libraries(honesty_main
	PUBLIC
	synodic::honesty
	synodic::portability
	synodic::periapsis
)

# Gather and define all the project and sub-project sources
add_subdirectory(
	src
)

# NOTE: For IDE setups that do not use CMake directly, we setup all IDE directories to mimic
# their system directory structure. See https://gitlab.kitware.com/cmake/cmake/-/issues/25992
file(GLOB_RECURSE file_list ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

# Must be called from the target definition directory. Only matched against 'target_sources'
# files (No nested CMakeList.txt, docs, etc).
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${file_list})

if(BUILD_TESTING AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
	# Do not include CTest because it adds targets and integrations we do not want to support
	enable_testing()

	add_subdirectory(
		tests
	)
endif()

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Generate package config files
write_basic_package_version_file(
	honestyConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
	tool/cmake/honestyConfig.cmake.in
	honestyConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/honesty
)

install(TARGETS honesty honesty_main
	EXPORT honestyTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}

	FILE_SET honesty_public DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/honesty
	FILE_SET honesty_private DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/honesty
)

# Create empty include directory to satisfy CMake's exported target requirements
# (module-only library with no headers, but CMake expects the directory to exist)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export and package configuration
install(EXPORT honestyTargets
	FILE honestyTargets.cmake
	NAMESPACE synodic::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/honesty
	CXX_MODULES_DIRECTORY cxx-modules
)

install(
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/honestyConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/honestyConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/honesty
)
