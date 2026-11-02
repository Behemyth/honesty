export module synodic.honesty.test:reporter;

import synodic.honesty.log;
import synodic.honesty.test.backend;

import std;

namespace honesty::test
{
	/**
	 * @brief Reports events immediately as they are signaled.
	 */
	export class StreamingAdapter : public Reporter
	{
	public:
		explicit(false) constexpr StreamingAdapter(const log::Logger& logger) :
			Reporter(logger)
		{
		}

		~StreamingAdapter() override = default;

		void Signal(const event::Initialize&) override
		{
		}

		void Signal(const event::SuiteBegin&) override
		{
		}

		void Signal(const event::SuiteEnd&) override
		{
		}

		void Signal(const event::TestBegin&) override
		{
		}

		void Signal(const event::TestEnd&) override
		{
		}

		void Signal(const event::TestSkip&) override
		{
		}

		void Signal(const event::AssertionFail&) override
		{
		}

		void Signal(const event::EqualityFail& event) override
		{
			// If the equality test is not overriden, we splice the input into a generic assertion fail
			Signal(static_cast<event::AssertionFail>(event));
		}

		void Signal(const event::ComparisonFail& event) override
		{
			// If the ordering test is not overriden, we splice the input into a generic assertion fail
			Signal(static_cast<event::AssertionFail>(event));
		}

		void Signal(const event::AssertionPass&) override
		{
		}

		void Signal(const event::AssertionSkip&) override
		{
		}

		void Signal(const event::Summary&) override
		{
		}

		void Signal(const event::BenchmarkComplete&) override
		{
		}

		void Signal(const event::BenchmarkGroupComplete&) override
		{
		}
	};

	/**
	 * @brief Collects test data in a cumulative manner then reports it when the test suite is complete.
	 */
	export class CumulativeAdapter : public Reporter
	{
	public:
		struct TestData
		{
			std::string name;

			std::vector<TestData> subTests;
		};

		struct SuiteData
		{
			std::string name;

			std::vector<TestData> tests;
		};

		struct CumulativeData
		{
			std::vector<SuiteData> suites;
		};

		explicit(false) CumulativeAdapter(const log::Logger& logger) :
			Reporter(logger)
		{
		}

		~CumulativeAdapter() override = default;

		void Signal(const event::Initialize&) final
		{
		}

		void Signal(const event::SuiteBegin& event) final
		{
			auto& [name, tests] = data_.suites.emplace_back();

			name = event.name;
		}

		void Signal(const event::SuiteEnd& event) final
		{
			auto& [name, tests] = data_.suites.back();

			name = event.name;
		}

		void Signal(const event::TestBegin& event) final
		{
			SuiteData& suiteData = data_.suites.back();
			TestData& testData	 = suiteData.tests.emplace_back();
			testData.name		 = event.name;
		}

		void Signal(const event::TestEnd& event) final
		{
			SuiteData& suiteData = data_.suites.back();
			TestData& testData	 = suiteData.tests.back();
			testData.name		 = event.name;
		}

		void Signal(const event::TestSkip&) final
		{
		}

		void Signal(const event::AssertionFail&) final
		{
		}

		void Signal(const event::EqualityFail&) final
		{
		}

		void Signal(const event::ComparisonFail&) final
		{
		}

		void Signal(const event::AssertionPass&) final
		{
		}

		void Signal(const event::AssertionSkip&) final
		{
		}

		void Signal(const event::Summary&) final
		{
			Finalize(std::move(data_));
		}

		void Signal(const event::BenchmarkComplete&) final
		{
		}

		void Signal(const event::BenchmarkGroupComplete&) final
		{
		}

		virtual void Finalize(CumulativeData data) = 0;

	private:
		CumulativeData data_;
	};

	template<typename T>
	concept reporter = requires {
		T::Name();
		{
			std::bool_constant<(T::Name(), true)>()
		} -> std::same_as<std::true_type>;
		std::derived_from<Reporter, T>;
	};

	export template<reporter T>
	class ReporterRegistrar final : ReporterRegistry
	{
	public:
		ReporterRegistrar()
		{
		}

		std::unique_ptr<Reporter> Create(const log::Logger& logger) const override
		{
			return std::make_unique<T>(logger);
		}

		std::string_view Name() const override
		{
			return T::Name();
		}
	};
}
