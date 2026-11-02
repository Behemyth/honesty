export module synodic.honesty.metric:context;

import std;
import function_ref;

import :types;
import :timer;
import :results;
import :platform;
import synodic.honesty.log;

namespace honesty::metric
{
	namespace
	{
		auto& MetricContextLogger()
		{
			static log::Logger logger = []() {
				log::Logger l = log::RootLogger().CreateLogger("honesty.metric.context");
				l.SetLevel(log::LevelType::WARNING);
				return l;
			}();
			return logger;
		}
	}

	/**
	 *	@brief Context for timed state. i.e. Run a measurement for a certain amount of time.
	 *
	 *	When MetricsEnabled is false (via -DHONESTY_METRICS_DISABLED), the Measure method
	 *	compiles to just executing the metric function once and returning empty results.
	 */
	export class TimedContext
	{
	public:
		/**
		 * @param minSampleDuration Minimum duration for each sample (determines iteration scaling)
		 * @param maxTotalDuration Maximum total time budget for measurement (default: 1 second)
		 */
		explicit TimedContext(
			const std::chrono::nanoseconds minSampleDuration,
			const std::chrono::nanoseconds maxTotalDuration = std::chrono::seconds(1)) :
			targetSampleDuration_(minSampleDuration),
			maxTotalDuration_(maxTotalDuration)
		{
		}

		Results Measure(const std::function_ref<void()> metric)
		{
			return MeasureImpl(metric, 1);
		}

	protected:
		/**
		 * @brief Internal measurement implementation
		 * @param metric The function to measure
		 * @param minIterationsPerSample Minimum iterations per sample
		 */
		Results MeasureImpl(
			const std::function_ref<void()> metric,
			std::uint32_t minIterationsPerSample)
		{
			if constexpr (!MetricsEnabled)
			{
				// When metrics are disabled, just run the function once (for side effects)
				// and return empty results. The optimizer will inline this to nearly nothing.
				metric();
				return Results{};
			}
			else
			{
				MetricContextLogger().Debug("Starting timed measurement (min sample: {}ns, max total: {}ns)",
					targetSampleDuration_.count(), maxTotalDuration_.count());

				State state(targetSampleDuration_, maxTotalDuration_, minIterationsPerSample);
				BenchmarkAccumulator accumulator;
				std::vector<double> rawSamples;
				rawSamples.reserve(State::maxSamples);

				// Each measurement depends on the previous sample.
				while (true)
				{
					// All iterations are measured under a single sample. Avoids the overhead of measuring each iteration
					Duration duration;

					// Scope the timer to just the executed operation
					{
						Timer timer(duration);

						while (state.Iterate())
						{
							metric();
						}
					}

					// Push normalized per-iteration time to accumulator and raw samples
					if (state.lastIterationCount > 0)
					{
						const double iterationTimeNs =
							static_cast<double>(duration.count()) / static_cast<double>(state.lastIterationCount);
						accumulator.Push(iterationTimeNs);
						rawSamples.push_back(iterationTimeNs);
					}

					if (not state.Push(duration))
					{
						break;
					}
				}

				Results results;
				results.FromAccumulatorWithSamples(accumulator, std::move(rawSamples), state.totalIterations, state.totalDuration);

				MetricContextLogger().Trace("Measurement complete: {} iterations, {} samples, median={}ns, MdAPE={:.2f}%",
					results.iterations, results.samples, results.median.count(), results.mdape * 100.0);

				return results;
			}
		}

	protected:
		/**
		 *	@brief Data that exists for the lifetime of a measurement
		 */
		struct State
		{
			/**
			 * @param minSampleDuration Minimum duration for each sample (based on clock resolution)
			 * @param maxTotalDuration Maximum total time budget for measurement
			 * @param minIterations Minimum iterations per sample
			 */
			explicit State(
				const std::chrono::nanoseconds minSampleDuration,
				const std::chrono::nanoseconds maxTotalDuration,
				const std::uint32_t minIterations = 1) :
				targetIterations(std::max(1u, minIterations)),
				lastIterationCount(0),
				totalIterations(0),
				sampleCount(0),
				totalDuration(0),
				minSampleDuration(minSampleDuration),
				maxTotalDuration(maxTotalDuration),
				minIterationsPerSample(std::max(1u, minIterations)),
				rng(std::random_device{}()),
				noiseDist(0.0, 0.2)  // 0-20% noise
			{
			}

			bool Iterate()
			{
				if (targetIterations > 0)
				{
					--targetIterations;
					++lastIterationCount;
					return true;
				}
				return false;
			}

			/**
			 *	@brief Updates the internal metric state with the sample from the last generation set
			 *	@return true if more samples should be collected, false if measurement is complete
			 */
			bool Push(const Duration& duration)
			{
				const auto elapsed = static_cast<double>(duration.count());
				totalDuration		+= duration;

				// lastIterationCount is updated during Iterate()
				totalIterations += lastIterationCount;
				++sampleCount;

				// Stop conditions:
				// 1. Collected maximum samples (for statistical robustness)
				// 2. Exceeded time budget (to prevent runaway measurements)
				if (sampleCount >= maxSamples || totalDuration >= maxTotalDuration)
				{
					return false;
				}

				// Calculate iterations for next sample to meet minimum sample duration
				// This is key for accuracy: each sample must be long enough relative to clock resolution
				std::uint32_t nextIterations = 1;
				if (elapsed > 0)
				{
					// Scale iterations so next sample targets minSampleDuration
					// If current sample took `elapsed` ns for `lastIterationCount` iterations,
					// we need (minSampleDuration / elapsed) * lastIterationCount iterations
					const double scale = static_cast<double>(minSampleDuration.count()) / elapsed;
					double baseIterations = scale * lastIterationCount;

					// Add 0-20% random noise to prevent aliasing effects
					// This technique is from nanobench and improves accuracy
					const double noise = 1.0 + noiseDist(rng);
					baseIterations *= noise;

					// Enforce minimum iterations per sample
					nextIterations = std::max(minIterationsPerSample, static_cast<std::uint32_t>(baseIterations));
				}

				// Reset for next sample
				lastIterationCount = 0;
				targetIterations = nextIterations;

				return true;
			}

			std::uint32_t targetIterations;
			std::uint32_t lastIterationCount;
			std::size_t totalIterations;
			std::size_t sampleCount;

			/// Maximum number of samples to collect (nanobench default is 1001)
			static constexpr std::size_t maxSamples = 1001;

			std::chrono::nanoseconds totalDuration;
			/// Minimum duration for each sample (clock resolution × multiplier)
			std::chrono::nanoseconds minSampleDuration;
			/// Maximum total measurement time budget
			std::chrono::nanoseconds maxTotalDuration;
			/// Minimum iterations per sample
			std::uint32_t minIterationsPerSample;

			// Random noise generator for anti-aliasing
			std::minstd_rand rng;
			std::uniform_real_distribution<double> noiseDist;
		};

		std::chrono::nanoseconds targetSampleDuration_;
		std::chrono::nanoseconds maxTotalDuration_;
	};

	/**
	 *	@brief Context for iterative state. i.e. Run a measurement for a certain number of iterations. Because the real
	 *		resolution of the underlying hardware clock isn't known we internally introduce a timed context to provide
	 *		enough variability to provide meaningful results, scaling the iterations accordingly.
	 */
	export class RegressionContext : public TimedContext
	{
	public:
		/**
		 * @brief Configuration for regression measurement
		 */
		struct Config
		{
			/// Number of warmup iterations before measurement (default: 0)
			std::uint64_t warmupIterations = 0;

			/// Platform tuning configuration
			PlatformConfig platform = {};

			/// Whether to apply platform tuning during measurement
			bool enablePlatformTuning = true;

			/// Clock resolution multiplier (higher = longer samples, more stable)
			/// nanobench uses 1000, which means each sample targets 1000x the clock resolution
			std::size_t clockResolutionMultiple = 1000;

			/// Maximum total measurement time (default: 1 second)
			std::chrono::nanoseconds maxTotalDuration = std::chrono::seconds(1);

			/// Minimum iterations per sample (prevents unreliable single-iteration samples)
			std::uint32_t minIterationsPerSample = 1;
		};

		/**
		 * @brief Initializes the context to an estimated minimal fit of iterations/samples
		 */
		explicit RegressionContext(Config config = {}) :
			TimedContext(GetClockResolution() * config.clockResolutionMultiple, config.maxTotalDuration),
			config_(std::move(config))
		{
			MetricContextLogger().Debug("Clock resolution: {}ns, min sample duration: {}ns, max total: {}ns",
				std::chrono::duration_cast<std::chrono::nanoseconds>(GetClockResolution()).count(),
				targetSampleDuration_.count(),
				maxTotalDuration_.count());
		}

		/**
		 * @brief Measures a function and stores the results internally for later retrieval.
		 *
		 * When MetricsEnabled is false, this compiles to just executing the metric function
		 * once and returning empty results.
		 *
		 * @param metric The function to measure
		 * @return The measurement results
		 */
		Results Measure(const std::function_ref<void()> metric)
		{
			if constexpr (!MetricsEnabled)
			{
				// When metrics are disabled, just run the function once and return empty results
				metric();
				lastResults_ = Results{};
				return lastResults_;
			}
			else
			{
				MetricContextLogger().Debug("Starting regression measurement");

				// Apply platform tuning if enabled
				std::optional<ScopedPlatformTuner> platformTuner;
				if (config_.enablePlatformTuning)
				{
					platformTuner.emplace(config_.platform);

					// Log any warnings from platform setup
					for (const auto& warning : platformTuner->State().warnings)
					{
						MetricContextLogger().Warning("Platform: {}", warning);
					}
				}

				// Run warmup iterations (not timed)
				if (config_.warmupIterations > 0)
				{
					MetricContextLogger().Debug("Running {} warmup iterations", config_.warmupIterations);
					for (std::uint64_t i = 0; i < config_.warmupIterations; ++i)
					{
						metric();
					}
				}

				// Perform actual measurement
				lastResults_ = TimedContext::MeasureImpl(metric, config_.minIterationsPerSample);

				MetricContextLogger().Debug("Regression measurement complete");
				return lastResults_;
			}
		}

		/**
		 * @brief Returns the results from the last Measure() call
		 * @return The last measurement results
		 */
		const Results& GetResults() const
		{
			return lastResults_;
		}

		/**
		 * @brief Get the current configuration
		 */
		const Config& GetConfig() const noexcept
		{
			return config_;
		}

		/**
		 * @brief Set the number of warmup iterations
		 * @param count Number of iterations to run before measurement
		 * @return Reference to this context for chaining
		 */
		RegressionContext& Warmup(std::uint64_t count) noexcept
		{
			config_.warmupIterations = count;
			return *this;
		}

		/**
		 * @brief Enable or disable platform tuning
		 * @param enable Whether to apply platform-specific optimizations
		 * @return Reference to this context for chaining
		 */
		RegressionContext& EnablePlatformTuning(bool enable) noexcept
		{
			config_.enablePlatformTuning = enable;
			return *this;
		}

		/**
		 * @brief Set the clock resolution multiplier
		 * @param multiple Target duration = clock resolution × multiple (default: 1000)
		 * @return Reference to this context for chaining
		 */
		RegressionContext& ClockResolutionMultiple(std::size_t multiple) noexcept
		{
			config_.clockResolutionMultiple = multiple;
			// Recalculate target duration
			targetSampleDuration_ = GetClockResolution() * multiple;
			return *this;
		}

		/**
		 * @brief Set minimum iterations per sample
		 * @param count Minimum iterations to run per sample (default: 1)
		 * @return Reference to this context for chaining
		 */
		RegressionContext& MinIterationsPerSample(std::uint32_t count) noexcept
		{
			config_.minIterationsPerSample = std::max(1u, count);
			return *this;
		}

	private:
		Config config_;
		Results lastResults_;
	};

}
