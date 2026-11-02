export module synodic.honesty.metric:results;

import synodic.periapsis;

import :types;
import std;

namespace honesty::metric
{
	/**
	 * @brief Accumulator type for collecting benchmark timing samples.
	 */
	export using BenchmarkAccumulator = peri::statistics::Accumulator<
		double,
		peri::statistics::Count,
		peri::statistics::Sum,
		peri::statistics::Mean,
		peri::statistics::Variance,
		peri::statistics::Min,
		peri::statistics::Max>;

	/**
	 * @brief A duration type using double representation for statistical calculations.
	 */
	using StatsDuration = std::chrono::duration<double, std::nano>;

	/**
	 * @brief Computes median from a vector of samples (modifies input for sorting)
	 */
	inline double ComputeMedian(std::vector<double>& samples)
	{
		if (samples.empty())
			return 0.0;

		const auto mid = samples.size() / 2;
		std::ranges::nth_element(samples, samples.begin() + mid);

		if (samples.size() % 2 == 0)
		{
			// Even count: average of two middle elements
			const auto maxLeft = *std::ranges::max_element(samples.begin(), samples.begin() + mid);
			return (maxLeft + samples[mid]) / 2.0;
		}
		return samples[mid];
	}

	/**
	 * @brief Computes MdAPE (Median Absolute Percentage Error) from samples
	 *
	 * MdAPE = median( |sample - median| / sample )
	 *
	 * This is more robust to outliers than standard deviation.
	 */
	inline double ComputeMdAPE(std::vector<double>& samples, double median)
	{
		if (samples.empty() || median == 0.0)
			return 0.0;

		std::vector<double> absolutePercentErrors;
		absolutePercentErrors.reserve(samples.size());

		for (const auto sample : samples)
		{
			if (sample > 0.0)
			{
				absolutePercentErrors.push_back(std::abs(sample - median) / sample);
			}
		}

		return ComputeMedian(absolutePercentErrors);
	}

	/**
	 * @brief Statistical results from benchmark measurements.
	 *
	 * Populated from BenchmarkAccumulator after measurement completion.
	 * Uses std::chrono types for type-safe duration representation.
	 */
	export struct Results
	{
		// Iteration and sample counts
		std::size_t iterations {};
		std::size_t samples {};

		// Total measurement duration
		Duration totalDuration {};

		// Statistical summary (per iteration)
		StatsDuration mean {};
		StatsDuration variance {};
		StatsDuration stddev {};
		StatsDuration min {};
		StatsDuration max {};

		// Robust statistics (per iteration)
		StatsDuration median {};	///< Median time per iteration (more robust than mean)
		double mdape {};			///< Median Absolute Percentage Error (0.0-1.0, more robust than stddev)

		// Derived metrics
		double iterations_per_second {};

		/**
		 * @brief Populate results from a BenchmarkAccumulator.
		 *
		 * @param accumulator The accumulator containing collected samples
		 * @param totalIterations Total number of benchmark iterations executed
		 * @param duration Total wall-clock duration of the benchmark
		 */
		void FromAccumulator(
			const BenchmarkAccumulator& accumulator,
			std::size_t totalIterations,
			Duration duration)
		{
			iterations    = totalIterations;
			samples       = accumulator.Get<peri::statistics::Count>().Result();
			totalDuration = duration;

			mean     = StatsDuration(accumulator.Get<peri::statistics::Mean>().Result());
			variance = StatsDuration(accumulator.Get<peri::statistics::Variance>().Result());
			stddev   = StatsDuration(accumulator.Get<peri::statistics::Variance>().StdDev());

			if (const auto minResult = accumulator.Get<peri::statistics::Min>().Result())
			{
				min = StatsDuration(*minResult);
			}

			if (const auto maxResult = accumulator.Get<peri::statistics::Max>().Result())
			{
				max = StatsDuration(*maxResult);
			}

			// Calculate throughput
			if (totalDuration.count() > 0)
			{
				iterations_per_second =
					static_cast<double>(iterations) * 1e9 / static_cast<double>(totalDuration.count());
			}
		}

		/**
		 * @brief Populate results including robust statistics from raw samples.
		 *
		 * @param accumulator The accumulator containing collected samples
		 * @param rawSamples Vector of individual per-iteration times (in nanoseconds)
		 * @param totalIterations Total number of benchmark iterations executed
		 * @param duration Total wall-clock duration of the benchmark
		 */
		void FromAccumulatorWithSamples(
			const BenchmarkAccumulator& accumulator,
			std::vector<double> rawSamples,
			std::size_t totalIterations,
			Duration duration)
		{
			// Populate standard statistics
			FromAccumulator(accumulator, totalIterations, duration);

			// Compute robust statistics from raw samples
			if (!rawSamples.empty())
			{
				const double medianValue = ComputeMedian(rawSamples);
				median = StatsDuration(medianValue);
				mdape = ComputeMdAPE(rawSamples, medianValue);
			}
		}
	};

}
