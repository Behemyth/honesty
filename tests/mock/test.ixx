export module synodic.honesty.test.mock;

import std;

import function_ref;

import synodic.honesty.metric;
import synodic.honesty.test;
import synodic.honesty.log;

using namespace honesty::test;

export class MockReporter final : public StreamingAdapter
{
public:
	explicit MockReporter(const honesty::log::Logger& logger) :
		StreamingAdapter(logger)
	{
	}

	static consteval std::string_view Name()
	{
		return "mock";
	}
};

/**
 * @brief A reporter that records all events for later inspection in tests.
 *
 * This reporter stores events in a variant vector, making it easy to verify
 * event sequences and event data without parsing string output.
 *
 * Note: Events containing string_view members are copied to owned strings
 * to ensure safe storage beyond the original event lifetime.
 */
export class RecordingReporter final : public StreamingAdapter
{
public:
	/**
	 * @brief Stored event types with owned string data for safe storage.
	 */
	struct StoredInitialize
	{
		std::uint64_t seed;
		std::string header;
	};

	struct StoredSuiteBegin
	{
		std::string name;
	};

	struct StoredSuiteEnd
	{
		std::string name;
		honesty::metric::Duration duration;
	};

	struct StoredTestBegin
	{
		std::string name;
		ExpectedAssertOutcome outcome;
	};

	struct StoredTestEnd
	{
		std::string name;
		honesty::metric::Duration duration;
	};

	struct StoredTestSkip
	{
		std::string name;
		bool todo;
	};

	struct StoredAssertionFail
	{
		bool exception;
		std::string message;
		std::source_location location;
	};

	struct StoredEqualityFail
	{
		bool exception;
		std::string message;
		std::source_location location;
		bool equal;
		std::string a;
		std::string b;
	};

	struct StoredComparisonFail
	{
		bool exception;
		std::string message;
		std::source_location location;
		std::strong_ordering ordering;
		std::string a;
		std::string b;
	};

	struct StoredAssertionPass
	{
		std::source_location location;
	};

	struct StoredAssertionSkip
	{
	};

	struct StoredSummary
	{
		honesty::metric::Duration duration;
	};

	using EventVariant = std::variant<
		StoredInitialize,
		StoredSuiteBegin,
		StoredSuiteEnd,
		StoredTestBegin,
		StoredTestEnd,
		StoredTestSkip,
		StoredAssertionFail,
		StoredEqualityFail,
		StoredComparisonFail,
		StoredAssertionPass,
		StoredAssertionSkip,
		StoredSummary>;

	explicit RecordingReporter(const honesty::log::Logger& logger) :
		StreamingAdapter(logger)
	{
	}

	static consteval std::string_view Name()
	{
		return "recording";
	}

	void Signal(const event::Initialize& event) override
	{
		events_.emplace_back(StoredInitialize{event.seed, std::string(event.header)});
	}

	void Signal(const event::SuiteBegin& event) override
	{
		events_.emplace_back(StoredSuiteBegin{std::string(event.name)});
	}

	void Signal(const event::SuiteEnd& event) override
	{
		events_.emplace_back(StoredSuiteEnd{std::string(event.name), event.duration});
	}

	void Signal(const event::TestBegin& event) override
	{
		events_.emplace_back(StoredTestBegin{std::string(event.name), event.outcome});
	}

	void Signal(const event::TestEnd& event) override
	{
		events_.emplace_back(StoredTestEnd{std::string(event.name), event.duration});
	}

	void Signal(const event::TestSkip& event) override
	{
		events_.emplace_back(StoredTestSkip{std::string(event.name), event.todo});
	}

	void Signal(const event::AssertionFail& event) override
	{
		events_.emplace_back(
			StoredAssertionFail{event.exception, std::string(event.message), event.location});
	}

	void Signal(const event::EqualityFail& event) override
	{
		events_.emplace_back(StoredEqualityFail{
			event.exception,
			std::string(event.message),
			event.location,
			event.equal,
			std::string(event.a),
			std::string(event.b)});
	}

	void Signal(const event::ComparisonFail& event) override
	{
		events_.emplace_back(StoredComparisonFail{
			event.exception,
			std::string(event.message),
			event.location,
			event.ordering,
			std::string(event.a),
			std::string(event.b)});
	}

	void Signal(const event::AssertionPass& event) override
	{
		events_.emplace_back(StoredAssertionPass{event.location});
	}

	void Signal(const event::AssertionSkip&) override
	{
		events_.emplace_back(StoredAssertionSkip{});
	}

	void Signal(const event::Summary& event) override
	{
		events_.emplace_back(StoredSummary{event.duration});
	}

	/**
	 * @brief Returns all recorded events.
	 */
	std::span<const EventVariant> Events() const
	{
		return events_;
	}

	/**
	 * @brief Clears all recorded events.
	 */
	void Clear()
	{
		events_.clear();
	}

	/**
	 * @brief Returns the number of recorded events.
	 */
	std::size_t Size() const
	{
		return events_.size();
	}

	/**
	 * @brief Checks if a specific event type exists in the recorded events.
	 */
	template<typename T>
	bool Contains() const
	{
		return std::ranges::any_of(
			events_,
			[](const EventVariant& event)
			{
				return std::holds_alternative<T>(event);
			});
	}

	/**
	 * @brief Counts the number of events of a specific type.
	 */
	template<typename T>
	std::size_t Count() const
	{
		return std::ranges::count_if(
			events_,
			[](const EventVariant& event)
			{
				return std::holds_alternative<T>(event);
			});
	}

private:
	std::vector<EventVariant> events_;
};

export class MockRunner final : public Runner
{
public:
	explicit MockRunner(const honesty::log::Logger& logger) :
		Runner(logger)
	{
	}

	static consteval std::string_view Name()
	{
		return "mock";
	}

	void Run(
		const Requirements& requirements,
		const std::function_ref<void(const Requirements&)> function) override
	{
		function(requirements);
	}

	Generator Run(
		const Requirements& requirements,
		const std::function_ref<Generator(const Requirements&)> function) override
	{
		return function(requirements);
	}

	Generator Run(const std::function_ref<Generator()> function) override
	{
		return function();
	}

	Generator Run(
		Fixture& fixture,
		const std::function_ref<Generator(Fixture&)> function) override
	{
		return function(fixture);
	}

	honesty::metric::Results Run(
		const std::function_ref<void(honesty::metric::RegressionContext&)> function) override
	{
		honesty::metric::RegressionContext context;
		function(context);
		return context.GetResults();
	}
};

namespace
{
	RunnerRegistrar<MockRunner> RUNNER_REGISTRAR;
	ReporterRegistrar<MockReporter> REPORTER_REGISTRAR;
}
