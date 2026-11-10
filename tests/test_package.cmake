if(NOT DEFINED TEST_CONFIG OR TEST_CONFIG STREQUAL "")
	set(TEST_CONFIG "${TEST_BUILD_TYPE}")
endif()

if(TEST_CONFIG STREQUAL "")
	set(TEST_CONFIG "Release")
endif()

string(TOLOWER "${TEST_CONFIG}" _config_key)
set(_test_root "${TEST_PROJECT_BINARY_DIR}/test_package/${_config_key}")
set(_install_prefix "${_test_root}/install")
set(_consumer_binary_dir "${_test_root}/consumer")
find_program(_ctest_command NAMES ctest REQUIRED)

file(REMOVE_RECURSE "${_test_root}")

execute_process(
	COMMAND "${CMAKE_COMMAND}" --install "${TEST_PROJECT_BINARY_DIR}"
		--prefix "${_install_prefix}" --config "${TEST_CONFIG}"
	RESULT_VARIABLE _install_result
)
if(NOT _install_result EQUAL 0)
	message(FATAL_ERROR "Installing the package for its consumer failed.")
endif()

set(_configure_command
	"${CMAKE_COMMAND}"
	-S "${TEST_SOURCE_DIR}"
	-B "${_consumer_binary_dir}"
	-G "${TEST_GENERATOR}"
	"-DCMAKE_PREFIX_PATH=${_install_prefix}"
)

if(TEST_GENERATOR_PLATFORM)
	list(APPEND _configure_command -A "${TEST_GENERATOR_PLATFORM}")
endif()
if(TEST_GENERATOR_TOOLSET)
	list(APPEND _configure_command -T "${TEST_GENERATOR_TOOLSET}")
endif()
if(TEST_TOOLCHAIN_FILE)
	list(APPEND _configure_command "-DCMAKE_TOOLCHAIN_FILE=${TEST_TOOLCHAIN_FILE}")
endif()
if(TEST_MAKE_PROGRAM)
	list(APPEND _configure_command "-DCMAKE_MAKE_PROGRAM=${TEST_MAKE_PROGRAM}")
endif()
# Build the consumer with the same compiler and flags as the package under test.
if(TEST_CXX_COMPILER AND NOT TEST_GENERATOR MATCHES "^Visual Studio")
	list(APPEND _configure_command "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}")
endif()
if(TEST_CXX_FLAGS)
	list(APPEND _configure_command "-DCMAKE_CXX_FLAGS=${TEST_CXX_FLAGS}")
endif()
if(TEST_EXE_LINKER_FLAGS)
	list(APPEND _configure_command "-DCMAKE_EXE_LINKER_FLAGS=${TEST_EXE_LINKER_FLAGS}")
endif()
if(TEST_CXX_STDLIB_MODULES_JSON)
	list(APPEND _configure_command "-DCMAKE_CXX_STDLIB_MODULES_JSON=${TEST_CXX_STDLIB_MODULES_JSON}")
endif()
# Give the consumer the package's prepared dependencies, as the package's own build has them.
if(TEST_TOP_LEVEL_INCLUDES)
	string(REPLACE "|" "\\;" _top_level_includes "${TEST_TOP_LEVEL_INCLUDES}")
	list(APPEND _configure_command "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${_top_level_includes}")
endif()
if(TEST_BUILD_IDENTITY)
	list(APPEND _configure_command "-DSYNODIC_BUILD_IDENTITY=${TEST_BUILD_IDENTITY}")
endif()
if(TEST_DEPENDENCIES_ROOT)
	list(APPEND _configure_command "-DSYNODIC_DEPENDENCIES_ROOT=${TEST_DEPENDENCIES_ROOT}")
endif()
# The installed package under test has only the tested configuration; its dependencies use the mapped one.
if(TEST_MAP_IMPORTED_CONFIG_DEVELOPMENT)
	list(APPEND _configure_command
		"-DCMAKE_MAP_IMPORTED_CONFIG_DEVELOPMENT=Development\\;${TEST_MAP_IMPORTED_CONFIG_DEVELOPMENT}"
	)
endif()
if(TEST_IS_MULTI_CONFIG AND TEST_CONFIG)
	list(APPEND _configure_command "-DCMAKE_CONFIGURATION_TYPES=${TEST_CONFIG}")
elseif(TEST_CONFIG)
	list(APPEND _configure_command "-DCMAKE_BUILD_TYPE=${TEST_CONFIG}")
endif()

execute_process(
	COMMAND ${_configure_command}
	RESULT_VARIABLE _configure_result
)
if(NOT _configure_result EQUAL 0)
	message(FATAL_ERROR "Configuring the installed-package consumer failed.")
endif()

execute_process(
	COMMAND "${CMAKE_COMMAND}" --build "${_consumer_binary_dir}"
		--config "${TEST_CONFIG}"
	RESULT_VARIABLE _build_result
)
if(NOT _build_result EQUAL 0)
	message(FATAL_ERROR "Building the installed-package consumer failed.")
endif()

execute_process(
	COMMAND "${_ctest_command}" --test-dir "${_consumer_binary_dir}"
		--build-config "${TEST_CONFIG}" --output-on-failure
	RESULT_VARIABLE _test_result
)
if(NOT _test_result EQUAL 0)
	message(FATAL_ERROR "Running the installed-package consumer failed.")
endif()
