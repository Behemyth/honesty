export module synodic.honesty.test.backend:fixture;

import std;

import synodic.honesty.log;

// TODO: Implement user fixture extensions.

/***
 * @brief Generates a random name suitable for a temporary data.
 * @param prefix The prefix to the name.
 * @param length The length of the random part of the name.
 * @return The generated name.
 */
std::string GenerateName(const std::string_view prefix, const std::size_t length = 32)
{
	std::string name(prefix);
	name += "_";

	// A distribution fitting 16 characters
	std::uniform_int_distribution distribution(0, 15);
	std::random_device device;

	name.reserve(name.size() + length);

	for (int iteration = 0; iteration < length; ++iteration)
	{
		name += "0123456789ABCDEF"[distribution(device)];
	}

	return name;
}

namespace honesty::test
{
	export class Reporter;

	export using TempLogger = log::ScopedLogger<log::OStream<std::mutex>>;

	/**
	 * @brief A set of thread-safe, stateful utilities for testing. Data fixtures can be implemented by the user's tests
	 *	themselves.
	 */
	export class Fixture final
	{
		friend struct SuiteContext;

	public:
		~Fixture()
		{
			log::Logger& logger = logger_.get();

			logger.RemoveSink();
		}

		Fixture(const Fixture& other)                = delete;
		Fixture(Fixture&& other) noexcept            = delete;
		Fixture& operator=(const Fixture& other)     = delete;
		Fixture& operator=(Fixture&& other) noexcept = delete;

		/**
		 * @brief Returns a path to a directory in the system's temporary directory. The directory is unique to the run
		 *	and suite name. When returned, the directory is created.
		 * @return The path to the directory.
		 */
		[[nodiscard]] auto SuiteDirectory() const -> std::filesystem::path
		{
			std::error_code code;
			create_directories(suiteTempDirectory_, code);

			if (code)
			{
				throw std::runtime_error("Failed to create directory: " + suiteTempDirectory_.generic_string());
			}

			return suiteTempDirectory_;
		}

		/**
		 * @brief Returns a randomly generated name suitable for a temporary file.
		 * @return A random filename.
		 */
		auto TempFileName() const -> std::string
		{
			return GenerateName(applicationName_) + ".tmp";
		}

		/**
		 * @brief A temporary logger is added to the thread's root logger and not the application's logger
		 */
		auto TempLog(std::ostream& stream) const -> TempLogger
		{
			const std::string name = GenerateName(suiteName_);

			log::Logger logger = log::RootLogger().CreateLogger(name);
			return TempLogger(std::move(logger), stream);
		}

		/**
		 * @brief Attaches a listener to the application's logger.
		 * @param level The log level to listen to.
		 */
		std::stringstream& AttachListener(const log::LevelType level = log::LevelType::INFO)
		{
			log::Logger& logger = logger_.get();

			// Filter our sink based on what the user wants to listen to
			streamSink_.SetLevel(level);

			logger.SetSink(&streamSink_);

			return stream_;
		}

		std::string_view ApplicationName() const
		{
			return applicationName_;
		}

		std::string_view SuiteName() const
		{
			return suiteName_;
		}

	private:
		/**
		 * @brief The parameters for the fixture. This is used to provide helpers to the suite.
		 */
		struct Parameters
		{
		};

		/**
		 * @brief Output provided by the fixture. Not accessible from the user's suite.
		 */
		struct Output
		{
		};

		/**
		 * @brief 
		 * @param reporters 
		 * @param applicationName 
		 * @param suiteName 
		 * @param logger A new logger object to take ownership of.
		 */
		Fixture(
			[[maybe_unused]] const std::span<Reporter*> reporters,
			const std::string_view applicationName,
			const std::string_view suiteName,
			log::Logger& logger) :
			applicationName_(applicationName),
			suiteName_(suiteName),
			logger_(logger),
			streamSink_(stream_)
		{
			const std::string suiteDirectory = GenerateName(suiteName_);

			suiteTempDirectory_ =
				std::filesystem::temp_directory_path() / "honesty" / applicationName_ / suiteDirectory;
		}

		std::string_view applicationName_;
		std::string_view suiteName_;
		std::filesystem::path suiteTempDirectory_;

		// Reference to the base logger for the suite
		std::reference_wrapper<log::Logger> logger_;

		log::OStream<std::mutex> streamSink_;
		std::stringstream stream_;
	};
}
