/**
 * @file macos.cpp
 * @brief macOS resource collection
 *
 * Implements resource collection using:
 * - getrusage for process CPU time
 * - task_info for current and peak resident memory
 */

module;

#include <mach/mach.h>
#include <sys/resource.h>

module synodic.honesty.metric;

import std;

namespace honesty::metric
{
	namespace
	{
		/// Process CPU time, user plus system
		std::chrono::microseconds ProcessCpuTime()
		{
			rusage usage{};
			if (getrusage(RUSAGE_SELF, &usage) != 0)
			{
				return std::chrono::microseconds::zero();
			}

			const auto toDuration = [](const timeval& time)
			{
				return std::chrono::seconds(time.tv_sec) + std::chrono::microseconds(time.tv_usec);
			};
			return toDuration(usage.ru_utime) + toDuration(usage.ru_stime);
		}
	}

	struct ResourceCollector::Impl
	{
		ResourceConfig config;
		bool initialized = false;

		// Previous times for CPU delta calculation
		std::chrono::microseconds prevProcessTime{0};
		Clock::time_point prevWallTime;

		explicit Impl(ResourceConfig cfg)
			: config(std::move(cfg))
		{
		}

		bool InitializeCpu()
		{
			if (!config.collectCpu)
			{
				return true;
			}

			prevProcessTime = ProcessCpuTime();
			prevWallTime = Clock::now();

			return true;
		}

		double SampleCpu()
		{
			if (!config.collectCpu)
			{
				return 0.0;
			}

			const std::chrono::microseconds currentProcessTime = ProcessCpuTime();
			const Clock::time_point currentWallTime = Clock::now();

			const std::chrono::duration<double> processTimeDelta = currentProcessTime - prevProcessTime;
			const std::chrono::duration<double> wallTimeDelta = currentWallTime - prevWallTime;

			// Update previous times for next sample
			prevProcessTime = currentProcessTime;
			prevWallTime = currentWallTime;

			if (wallTimeDelta.count() <= 0.0)
			{
				return 0.0;
			}

			// Calculate CPU percentage
			// Note: This can exceed 100% on multi-core systems
			return (processTimeDelta.count() / wallTimeDelta.count()) * 100.0;
		}
	};

	ResourceCollector::ResourceCollector(ResourceConfig config)
		: impl_(std::make_unique<Impl>(std::move(config)))
	{
	}

	ResourceCollector::~ResourceCollector() = default;

	ResourceCollector::ResourceCollector(ResourceCollector&&) noexcept = default;
	ResourceCollector& ResourceCollector::operator=(ResourceCollector&&) noexcept = default;

	bool ResourceCollector::Initialize()
	{
		if (impl_->initialized)
		{
			return true;
		}

		if (!impl_->InitializeCpu())
		{
			return false;
		}

		impl_->initialized = true;
		return true;
	}

	ResourceSample ResourceCollector::Sample()
	{
		ResourceSample sample;
		sample.timestamp = Clock::now();

		if (!impl_->initialized)
		{
			return sample;
		}

		// CPU usage
		sample.cpuUsagePercent = impl_->SampleCpu();

		// Memory usage
		if (impl_->config.collectMemory)
		{
			mach_task_basic_info info{};
			mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
			if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count)
				== KERN_SUCCESS)
			{
				sample.memoryUsageBytes = info.resident_size;
				sample.peakMemoryBytes = info.resident_size_max;
			}
		}

		return sample;
	}

	bool ResourceCollector::IsInitialized() const noexcept
	{
		return impl_->initialized;
	}

	const ResourceConfig& ResourceCollector::Config() const noexcept
	{
		return impl_->config;
	}

	// ScopedResourceSampler implementation

	ScopedResourceSampler::ScopedResourceSampler(ResourceCollector& collector)
		: collector_(collector)
		, startSample_(collector.Sample())
	{
	}

	const ResourceSample& ScopedResourceSampler::StartSample() const noexcept
	{
		return startSample_;
	}

	ResourceSample ScopedResourceSampler::End()
	{
		ResourceSample endSample = collector_.Sample();

		// Compute delta sample
		ResourceSample delta;
		delta.timestamp = endSample.timestamp;
		delta.cpuUsagePercent = endSample.cpuUsagePercent; // CPU is already a rate
		delta.memoryUsageBytes = endSample.memoryUsageBytes - startSample_.memoryUsageBytes;
		delta.peakMemoryBytes = endSample.peakMemoryBytes;

		return delta;
	}
}
