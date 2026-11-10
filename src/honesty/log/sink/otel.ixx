module;

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/severity.h>

// Windows workaround
#undef ERROR

export module synodic.honesty.log:sink.otel;

import std;
import :colour;
import :sink.synchronized;
import :types;

namespace honesty::log
{
	/**
	 * @brief Initialize global OpenTelemetry logging with ostream exporter
	 *
	 * Sets up an OStream exporter for debugging/testing.
	 * Call this once at application startup before emitting any logs.
	 */
	export void InitOTelLogging();

	/**
	 * @brief Cleanup global OpenTelemetry logging
	 *
	 * Resets the global logger provider. Call at application shutdown.
	 */
	export void CleanupOTelLogging();

	export template<mutex Mutex>
	class OTelemetry final : public SynchronizedSink<Mutex>
	{
	public:
		void SynchronizedLogV(const LevelType level, const std::string_view fmt, const std::format_args args) override
		{
			const auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
			const auto logger = provider->GetLogger("honesty");

			if (auto logRecord = logger->CreateLogRecord())
			{
				logRecord->SetSeverity(OTelLevel(level));

				std::string result = std::vformat(fmt, args);
				logRecord->SetBody(opentelemetry::nostd::string_view(result.data(), result.size()));

				logger->EmitLogRecord(std::move(logRecord));
			}
		}

		void SynchronizedFlush() override
		{
		}

		static opentelemetry::logs::Severity OTelLevel(const LevelType level)
		{
			switch (level)
			{
				case LevelType::OFF :
				case LevelType::DEFER :
				case LevelType::COUNT :
				{
					return opentelemetry::logs::Severity::kInvalid;
				}
				case LevelType::TRACE :
				{
					return opentelemetry::logs::Severity::kTrace;
				}
				case LevelType::DEBUG :
				{
					return opentelemetry::logs::Severity::kDebug;
				}
				case LevelType::INFO :
				{
					return opentelemetry::logs::Severity::kInfo;
				}
				case LevelType::WARNING :
				{
					return opentelemetry::logs::Severity::kWarn;
				}
				case LevelType::ERROR :
				{
					return opentelemetry::logs::Severity::kError;
				}
				case LevelType::CRITICAL :
				{
					return opentelemetry::logs::Severity::kFatal;
				}
			}

			return opentelemetry::logs::Severity::kInvalid;
		}
	};
}
