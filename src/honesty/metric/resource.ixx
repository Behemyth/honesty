/**
 * @file resource.ixx
 * @brief System resource collection for benchmarking and observability
 *
 * Provides point-in-time sampling of system resource usage (CPU, memory).
 * Platform-specific implementations use:
 * - Windows: process time and memory APIs
 * - Linux: getrusage and the /proc filesystem
 * - macOS: getrusage and task_info
 */

export module synodic.honesty.metric:resource;

import :types;
import std;

namespace honesty::metric
{
	/**
	 * @brief A point-in-time sample of system resource usage
	 */
	export struct ResourceSample
	{
		/// Timestamp when the sample was taken
		Clock::time_point timestamp;

		/// Process CPU usage as a percentage (0.0 - 100.0+)
		/// May exceed 100% on multi-core systems if measuring all cores
		double cpuUsagePercent = 0.0;

		/// Current process memory usage in bytes (working set / RSS)
		std::size_t memoryUsageBytes = 0;

		/// Peak process memory usage in bytes
		std::size_t peakMemoryBytes = 0;
	};

	/**
	 * @brief Configuration for resource collection
	 */
	export struct ResourceConfig
	{
		/// Whether to collect CPU usage metrics
		bool collectCpu = true;

		/// Whether to collect memory usage metrics
		bool collectMemory = true;
	};

	/**
	 * @brief Collects system resource metrics for the current process
	 *
	 * This class provides point-in-time sampling of CPU and memory usage.
	 * Each call to Sample() returns the current resource state.
	 *
	 * Platform implementations:
	 * - Windows: Uses GetProcessTimes for CPU, GetProcessMemoryInfo for memory
	 * - Linux: Uses getrusage for CPU, /proc/self/status for memory
	 * - macOS: Uses getrusage for CPU, task_info for memory
	 *
	 * Thread Safety: Each thread should have its own ResourceCollector instance.
	 * The collector maintains internal state for CPU calculation that is not thread-safe.
	 *
	 * Example usage:
	 * @code
	 * ResourceCollector collector;
	 * collector.Initialize();
	 *
	 * // ... run benchmark ...
	 *
	 * auto sample = collector.Sample();
	 * std::println("CPU: {}%, Memory: {} bytes", sample.cpuUsagePercent, sample.memoryUsageBytes);
	 * @endcode
	 */
	export class ResourceCollector
	{
	public:
		explicit ResourceCollector(ResourceConfig config = {});
		~ResourceCollector();

		// Non-copyable, movable
		ResourceCollector(const ResourceCollector&) = delete;
		ResourceCollector& operator=(const ResourceCollector&) = delete;
		ResourceCollector(ResourceCollector&&) noexcept;
		ResourceCollector& operator=(ResourceCollector&&) noexcept;

		/**
		 * @brief Initialize the resource collector
		 *
		 * Must be called before Sample(). Sets up platform-specific
		 * handles and baseline measurements for CPU calculation.
		 *
		 * @return true if initialization succeeded, false otherwise
		 */
		bool Initialize();

		/**
		 * @brief Take a point-in-time sample of current resource usage
		 *
		 * CPU usage is calculated as the delta since the last Sample() call
		 * (or Initialize() if this is the first sample).
		 *
		 * @return Current resource sample
		 */
		ResourceSample Sample();

		/**
		 * @brief Check if the collector has been initialized
		 */
		[[nodiscard]] bool IsInitialized() const noexcept;

		/**
		 * @brief Get the configuration
		 */
		[[nodiscard]] const ResourceConfig& Config() const noexcept;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

	/**
	 * @brief RAII wrapper that samples resources at construction and destruction
	 *
	 * Useful for measuring resource usage across a scope:
	 * @code
	 * {
	 *     ScopedResourceSampler sampler(collector);
	 *     // ... code to measure ...
	 * } // sampler.End() returns delta
	 * @endcode
	 */
	export class ScopedResourceSampler
	{
	public:
		explicit ScopedResourceSampler(ResourceCollector& collector);
		~ScopedResourceSampler() = default;

		// Non-copyable, non-movable
		ScopedResourceSampler(const ScopedResourceSampler&) = delete;
		ScopedResourceSampler& operator=(const ScopedResourceSampler&) = delete;
		ScopedResourceSampler(ScopedResourceSampler&&) = delete;
		ScopedResourceSampler& operator=(ScopedResourceSampler&&) = delete;

		/**
		 * @brief Get the sample taken at construction
		 */
		[[nodiscard]] const ResourceSample& StartSample() const noexcept;

		/**
		 * @brief Take end sample and compute resource delta
		 *
		 * @return Sample representing resource usage during the scope
		 */
		ResourceSample End();

	private:
		ResourceCollector& collector_;
		ResourceSample startSample_;
	};
}
