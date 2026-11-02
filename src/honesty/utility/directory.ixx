/**
 * @file directory.ixx
 * @brief Utilities for determining application data directories using std::filesystem
 *
 * Provides cross-platform support for locating persistent storage directories
 * for benchmark history and other application data.
 */

export module synodic.honesty.utility:directory;

import std;

namespace honesty::utility
{
	namespace
	{
		// The result is used immediately and never stored, so MSVC's getenv deprecation does not apply
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
		auto Environment(const char* name) -> const char*
		{
			return std::getenv(name);
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	}

	/**
	 * @brief Get the base directory for storing Honesty application data.
	 *
	 * Directory resolution order:
	 * 1. HONESTY_DATA_DIR environment variable (if set)
	 * 2. Platform-specific user data directory:
	 *    - Windows: %LOCALAPPDATA%/honesty
	 *    - Linux: $XDG_DATA_HOME/honesty or ~/.local/share/honesty
	 *    - macOS: ~/Library/Application Support/honesty
	 *
	 * @return Path to the Honesty data directory
	 * @throws std::runtime_error if no valid directory can be determined
	 */
	export auto DataDirectory() -> std::filesystem::path
	{
		// Check environment variable first
		if (const char* envDir = Environment("HONESTY_DATA_DIR"))
		{
			return std::filesystem::path(envDir);
		}

#if defined(_WIN32)
		// Windows: Use LOCALAPPDATA
		if (const char* localAppData = Environment("LOCALAPPDATA"))
		{
			return std::filesystem::path(localAppData) / "honesty";
		}
		// Fallback to APPDATA if LOCALAPPDATA is not available
		if (const char* appData = Environment("APPDATA"))
		{
			return std::filesystem::path(appData) / "honesty";
		}
#elif defined(__APPLE__)
		// macOS: Use ~/Library/Application Support
		if (const char* home = Environment("HOME"))
		{
			return std::filesystem::path(home) / "Library" / "Application Support" / "honesty";
		}
#else
		// Linux/Unix: Use XDG_DATA_HOME or fallback to ~/.local/share
		if (const char* xdgData = Environment("XDG_DATA_HOME"))
		{
			return std::filesystem::path(xdgData) / "honesty";
		}
		if (const char* home = Environment("HOME"))
		{
			return std::filesystem::path(home) / ".local" / "share" / "honesty";
		}
#endif

		// Last resort fallback: use temp directory with consistent path
		return std::filesystem::temp_directory_path() / "honesty";
	}

	/**
	 * @brief Get the directory for storing benchmark history data.
	 *
	 * Creates the directory if it doesn't exist.
	 *
	 * @return Path to the benchmarks directory
	 */
	export auto BenchmarkDirectory() -> std::filesystem::path
	{
		const auto dir = DataDirectory() / "benchmarks";

		std::error_code ec;
		std::filesystem::create_directories(dir, ec);

		if (ec)
		{
			throw std::runtime_error(std::format("Failed to create benchmark directory '{}': {}", dir.string(), ec.message()));
		}

		return dir;
	}

	/**
	 * @brief Ensure a directory exists, creating it if necessary.
	 *
	 * @param path The directory path to ensure exists
	 * @throws std::runtime_error if directory cannot be created
	 */
	export void EnsureDirectory(const std::filesystem::path& path)
	{
		std::error_code ec;
		std::filesystem::create_directories(path, ec);

		if (ec)
		{
			throw std::runtime_error(std::format("Failed to create directory '{}': {}", path.string(), ec.message()));
		}
	}
}
