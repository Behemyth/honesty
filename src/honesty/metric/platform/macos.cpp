/**
 * @file macos.cpp
 * @brief macOS platform tuning for benchmark stability
 *
 * No tuning is applied on macOS yet. The tuner keeps its lifecycle so callers
 * behave the same on every platform, and reports that nothing was applied.
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

			isActive = true;
			state.tuningApplied = false;
			PlatformLogger().Debug("macOS platform tuning is not implemented; no tuning applied");

			return state;
		}

		void Teardown()
		{
			// Nothing to restore: Setup() changes no system state
			isActive = false;
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
