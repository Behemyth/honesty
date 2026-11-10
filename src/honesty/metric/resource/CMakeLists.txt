# Platform-specific resource collection implementations
if(WIN32)
	target_sources(honesty PRIVATE windows.cpp)
	# Link psapi library for GetProcessMemoryInfo
	target_link_libraries(honesty PRIVATE psapi)
elseif(APPLE)
	target_sources(honesty PRIVATE macos.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	target_sources(honesty PRIVATE linux.cpp)
else()
	message(FATAL_ERROR "Resource collection not yet implemented for this platform")
endif()