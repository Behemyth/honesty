/**
 * @file test_compile_time_config.cpp
 * @brief Tests for compile-time configuration of logging, metrics, and tracing.
 *
 * These tests verify that the compile-time configuration mechanism works correctly.
 * When features are disabled via build defines, the code should compile to nothing.
 *
 * Build defines:
 *   -DHONESTY_LOG_LEVEL_OFF         -> Disable all logging
 *   -DHONESTY_LOG_LEVEL_<LEVEL>     -> Set minimum log level
 *   -DHONESTY_METRICS_DISABLED      -> Disable all metrics
 *   -DHONESTY_TRACING_DISABLED      -> Disable all tracing
 *   -DHONESTY_TRACING_DEBUG_ONLY    -> Tracing only in debug builds
 */

import std;
import synodic.honesty.test;
import synodic.honesty.log;
import synodic.honesty.metric;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"compile_time_config",
		[]() -> Generator
		{
			// =================================================================
			// Logging compile-time configuration tests
			// =================================================================

			co_yield "log_compile_time_level"_test = [](const Requirements& requirements)
			{
				// Verify the compile-time log level is accessible
				[[maybe_unused]] constexpr auto level = honesty::log::CompileTimeLogLevel;

				// IsLevelEnabled should be consteval
				static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::CRITICAL) ||
				              honesty::log::CompileTimeLogLevel == honesty::log::LevelType::OFF);

				requirements.Expect(true);
			};

			co_yield "log_level_filtering"_test = [](const Requirements& requirements)
			{
				// When CompileTimeLogLevel is TRACE (default), all levels should be enabled
				if constexpr (honesty::log::CompileTimeLogLevel == honesty::log::LevelType::TRACE)
				{
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::TRACE));
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::DEBUG));
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::INFO));
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::WARNING));
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::ERROR));
					static_assert(honesty::log::IsLevelEnabled(honesty::log::LevelType::CRITICAL));
				}

				// When CompileTimeLogLevel is OFF, nothing should be enabled
				if constexpr (honesty::log::CompileTimeLogLevel == honesty::log::LevelType::OFF)
				{
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::TRACE));
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::DEBUG));
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::INFO));
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::WARNING));
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::ERROR));
					static_assert(!honesty::log::IsLevelEnabled(honesty::log::LevelType::CRITICAL));
				}

				requirements.Expect(true);
			};

			// =================================================================
			// Metrics compile-time configuration tests
			// =================================================================

			co_yield "metrics_compile_time_enabled"_test = [](const Requirements& requirements)
			{
				// Verify the compile-time metrics flag is accessible
				constexpr bool enabled = honesty::metric::MetricsEnabled;

				if constexpr (enabled)
				{
					// When enabled, Measure() should perform actual measurement
					honesty::metric::TimedContext context(std::chrono::milliseconds(1));
					int counter = 0;
					auto results = context.Measure([&counter]() { ++counter; });

					// Should have run multiple iterations
					requirements.Expect(results.iterations > 0);
					requirements.Expect(counter > 0);
				}
				else
				{
					// When disabled, Measure() should run function once and return empty results
					honesty::metric::TimedContext context(std::chrono::milliseconds(1));
					int counter = 0;
					auto results = context.Measure([&counter]() { ++counter; });

					// Should have run exactly once
					requirements.ExpectEquals(counter, 1);
					requirements.ExpectEquals(results.iterations, std::size_t{0});
				}
			};

			// =================================================================
			// Tracing compile-time configuration tests
			// =================================================================

			co_yield "tracing_compile_time_enabled"_test = [](const Requirements& requirements)
			{
				// Verify the compile-time tracing flag is accessible
				constexpr bool enabled = honesty::trace::TracingEnabled;

				honesty::trace::Tracer tracer;

				if constexpr (enabled)
				{
					// When enabled, CreateSpan() returns a real Span
					auto span = tracer.CreateSpan("test");
					static_assert(std::is_same_v<decltype(span), honesty::Span>);
					requirements.Expect(span.Data().traceID.IsValid());
				}
				else
				{
					// When disabled, CreateSpan() returns NoopSpan
					auto span = tracer.CreateSpan("test");
					static_assert(std::is_same_v<decltype(span), honesty::NoopSpan>);
					requirements.Expect(true);
				}
			};

			co_yield "tracer_is_enabled_consteval"_test = [](const Requirements& requirements)
			{
				// Tracer::IsEnabled() should be consteval
				static_assert(honesty::trace::Tracer::IsEnabled() == honesty::trace::TracingEnabled);
				requirements.Expect(true);
			};

			// =================================================================
			// Zero-cost verification tests
			// =================================================================

			co_yield "noop_types_are_empty"_test = [](const Requirements& requirements)
			{
				// NoopSpan should be an empty class (size 1 due to C++ rules)
				requirements.Expect(sizeof(honesty::NoopSpan) <= 1);

				// NoopContextGuard should be an empty class
				requirements.Expect(sizeof(honesty::trace::NoopContextGuard) <= 1);
			};
		});

	SuiteRegistrar _(SUITE);
}
