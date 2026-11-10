/**
 * @file linux.cpp
 * @brief Linux platform checks for benchmark stability
 *
 * Linux provides excellent tuning via external tools like `pyperf system tune`.
 * This implementation provides warnings and recommendations rather than
 * attempting system-level changes that typically require root privileges.
 *
 * For best results on Linux, run:
 *   sudo python3 -m pyperf system tune
 *
 * This will:
 * - Set CPU governor to 'performance'
 * - Disable turbo boost
 * - Set appropriate kernel parameters
 */

module synodic.honesty.metric;

import std;
import synodic.honesty.log;

namespace honesty::metric
{
	namespace
	{
		auto& PlatformLogger()
		{
			static log::Logger logger = []()
			{
				log::Logger l = log::RootLogger().CreateLogger("honesty.metric.platform");
				l.SetLevel(log::LevelType::DEBUG);
				return l;
			}();
			return logger;
		}

		/// Read a file and return its contents, or empty optional on failure
		std::optional<std::string> ReadProcFile(const std::filesystem::path& path)
		{
			std::error_code ec;
			if (!std::filesystem::exists(path, ec))
			{
				return std::nullopt;
			}

			std::ifstream file(path);
			if (!file)
			{
				return std::nullopt;
			}

			std::string content;
			std::getline(file, content);
			return content;
		}

		/// Check CPU governor for all cores
		void CheckCpuGovernor(std::vector<std::string>& warnings, bool& recommendPyperf)
		{
			// Check first CPU's governor as representative
			auto governor = ReadProcFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
			if (governor && *governor != "performance")
			{
				warnings.emplace_back(
					std::format("CPU governor is '{}' but should be 'performance'", *governor));
				recommendPyperf = true;
			}
		}

		/// Check for turbo boost status
		void CheckTurboBoost(std::vector<std::string>& warnings, bool& recommendPyperf)
		{
			// Intel pstate driver
			auto intelNoTurbo = ReadProcFile("/sys/devices/system/cpu/intel_pstate/no_turbo");
			if (intelNoTurbo && *intelNoTurbo == "0")
			{
				warnings.emplace_back("Intel Turbo Boost is enabled (may cause variance)");
				recommendPyperf = true;
			}

			// AMD boost
			auto amdBoost = ReadProcFile("/sys/devices/system/cpu/cpufreq/boost");
			if (amdBoost && *amdBoost == "1")
			{
				warnings.emplace_back("AMD Boost is enabled (may cause variance)");
				recommendPyperf = true;
			}
		}

		/// Check CPU frequency scaling
		void CheckFrequencyScaling(std::vector<std::string>& warnings, bool& recommendPyperf)
		{
			auto minFreq = ReadProcFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq");
			auto maxFreq = ReadProcFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq");

			if (minFreq && maxFreq)
			{
				try
				{
					auto min = std::stoul(*minFreq);
					auto max = std::stoul(*maxFreq);
					if (min != max)
					{
						warnings.emplace_back(
							std::format("CPU frequency scaling active: {} - {} kHz", min, max));
						recommendPyperf = true;
					}
				}
				catch (...)
				{
					// Ignore parse errors
				}
			}
		}
	}

	struct PlatformTuner::Impl
	{
		PlatformConfig config;
		bool isActive = false;

		explicit Impl(PlatformConfig cfg)
			: config(std::move(cfg))
		{
		}

		PlatformState Setup()
		{
			PlatformState state;

			if (isActive)
			{
				state.warnings.emplace_back("Platform tuning already active");
				return state;
			}

			PlatformLogger().Debug("Checking Linux platform stability");

			bool recommendPyperf = false;

			// Gather stability information
			CheckCpuGovernor(state.warnings, recommendPyperf);
			CheckTurboBoost(state.warnings, recommendPyperf);
			CheckFrequencyScaling(state.warnings, recommendPyperf);

			// Add pyperf recommendation if any issues found
			if (recommendPyperf)
			{
				state.recommendations.emplace_back(
					"For stable benchmarks, run: sudo python3 -m pyperf system tune");
			}

			// Log warnings
			for (const auto& warning : state.warnings)
			{
				PlatformLogger().Warning("{}", warning);
			}

			isActive = true;
			
			// Linux stub doesn't apply any tuning - it just reports
			state.tuningApplied = false;

			if (!state.warnings.empty())
			{
				PlatformLogger().Info(
					"Linux platform check complete: {} warnings, {} recommendations",
					state.warnings.size(), state.recommendations.size());
			}
			else
			{
				PlatformLogger().Debug("Linux platform check complete: system appears well-tuned");
			}

			return state;
		}

		void Teardown()
		{
			if (!isActive)
			{
				return;
			}

			// Nothing to restore on Linux - we didn't change anything
			isActive = false;
			PlatformLogger().Debug("Linux platform tuning teardown complete (no-op)");
		}
	};

	// PlatformTuner implementation

	PlatformTuner::PlatformTuner(PlatformConfig config)
		: impl_(std::make_unique<Impl>(std::move(config)))
	{
	}

	PlatformTuner::~PlatformTuner()
	{
		if (impl_)
		{
			impl_->Teardown();
		}
	}

	PlatformTuner::PlatformTuner(PlatformTuner&&) noexcept = default;
	PlatformTuner& PlatformTuner::operator=(PlatformTuner&&) noexcept = default;

	PlatformState PlatformTuner::Setup()
	{
		return impl_->Setup();
	}

	void PlatformTuner::Teardown()
	{
		impl_->Teardown();
	}

	bool PlatformTuner::IsActive() const noexcept
	{
		return impl_ && impl_->isActive;
	}

	const PlatformConfig& PlatformTuner::Config() const noexcept
	{
		return impl_->config;
	}

	// ScopedPlatformTuner implementation

	ScopedPlatformTuner::ScopedPlatformTuner(PlatformConfig config)
		: tuner_(std::move(config))
		, state_(tuner_.Setup())
	{
	}

	ScopedPlatformTuner::~ScopedPlatformTuner()
	{
		tuner_.Teardown();
	}

	const PlatformState& ScopedPlatformTuner::State() const noexcept
	{
		return state_;
	}

	PlatformTuner& ScopedPlatformTuner::Tuner() noexcept
	{
		return tuner_;
	}
}

