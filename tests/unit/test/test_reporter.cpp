import std;
import synodic.honesty.test;
import synodic.honesty.test.api;
import synodic.honesty.test.mock;
import synodic.honesty.log;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"reporter",
		[](Fixture&) -> Generator
		{
			co_yield "empty_description"_test = [&](const Requirements& requirements)
			{
				std::stringstream stream;
				honesty::log::OStream<std::mutex> sink(stream);
				sink.SetLevel(honesty::log::LevelType::INFO);

				const honesty::log::Logger& root = honesty::log::RootLogger();
				honesty::log::Logger logger      = root.CreateLogger("reporter_test");
				logger.SetSink(&sink);

				StandardReporter reporter(logger);

				// Simulate the full event sequence for a test failure with an empty description
				const event::SuiteBegin suiteBeginEvent{"suite_name"};
				reporter.Signal(suiteBeginEvent);

				const event::TestBegin testBeginEvent("test_name", ExpectedAssertOutcome::PASS);
				reporter.Signal(testBeginEvent);

				const event::AssertionFail failEvent(std::source_location::current(), "", false);
				reporter.Signal(failEvent);

				const event::TestEnd testEndEvent{"test_name", {}};
				reporter.Signal(testEndEvent);

				const event::SuiteEnd suiteEndEvent{"suite_name", {}};
				reporter.Signal(suiteEndEvent);

				requirements.Expect(
					not stream.view().contains("Description:"),
					"Empty descriptions should not be displayed in test output");
			};

			co_yield "provided_description"_test = [&](const Requirements& requirements)
			{
				std::stringstream stream;
				honesty::log::OStream<std::mutex> sink(stream);
				sink.SetLevel(honesty::log::LevelType::INFO);

				const honesty::log::Logger& root = honesty::log::RootLogger();
				honesty::log::Logger logger      = root.CreateLogger("reporter_test");
				logger.SetSink(&sink);

				StandardReporter reporter(logger);

				// Simulate the full event sequence for a test failure with a description
				const event::SuiteBegin suiteBeginEvent{"suite_name"};
				reporter.Signal(suiteBeginEvent);

				const event::TestBegin testBeginEvent("test_name", ExpectedAssertOutcome::PASS);
				reporter.Signal(testBeginEvent);

				const event::AssertionFail failEvent(
					std::source_location::current(),
					"Custom error message",
					false);
				reporter.Signal(failEvent);

				const event::TestEnd testEndEvent{"test_name", {}};
				reporter.Signal(testEndEvent);

				const event::SuiteEnd suiteEndEvent{"suite_name", {}};
				reporter.Signal(suiteEndEvent);

				requirements.Expect(
					stream.view().contains("Description:"),
					"Provided descriptions should be displayed in test output");

				requirements.Expect(
					stream.view().contains("Custom error message"),
					"The custom description text should appear in output");
			};

			co_yield "recording_reporter"_test = [&](const Requirements& requirements)
			{
				const honesty::log::Logger& root = honesty::log::RootLogger();
				honesty::log::Logger logger      = root.CreateLogger("recording_test");

				RecordingReporter reporter(logger);

				// Signal a sequence of events
				const event::SuiteBegin suiteBeginEvent{"test_suite"};
				reporter.Signal(suiteBeginEvent);

				const event::TestBegin testBeginEvent("test_case", ExpectedAssertOutcome::PASS);
				reporter.Signal(testBeginEvent);

				const event::AssertionPass passEvent(std::source_location::current());
				reporter.Signal(passEvent);

				const event::AssertionFail failEvent(
					std::source_location::current(),
					"Test failure message",
					false);
				reporter.Signal(failEvent);

				const event::TestEnd testEndEvent{"test_case", {}};
				reporter.Signal(testEndEvent);

				const event::SuiteEnd suiteEndEvent{"test_suite", {}};
				reporter.Signal(suiteEndEvent);

				// Verify events were recorded
				requirements.ExpectEquals(reporter.Size(), 6uz, "Should record 6 events");

				requirements.Expect(
					reporter.Contains<RecordingReporter::StoredSuiteBegin>(),
					"Should contain SuiteBegin event");

				requirements.Expect(
					reporter.Contains<RecordingReporter::StoredAssertionPass>(),
					"Should contain AssertionPass event");

				requirements.Expect(
					reporter.Contains<RecordingReporter::StoredAssertionFail>(),
					"Should contain AssertionFail event");

				requirements.ExpectEquals(
					reporter.Count<RecordingReporter::StoredAssertionFail>(),
					1uz,
					"Should have exactly one assertion failure");

				// Verify event data is stored correctly
				const auto& events = reporter.Events();
				const auto* failEventStored =
					std::get_if<RecordingReporter::StoredAssertionFail>(&events[3]);

				requirements.Expect(failEventStored != nullptr, "Fourth event should be AssertionFail");

				if (failEventStored)
				{
					requirements.ExpectEquals(
						failEventStored->message,
						std::string("Test failure message"),
						"Failure message should be stored correctly");
				}

				// Test Clear functionality
				reporter.Clear();
				requirements.ExpectEquals(reporter.Size(), 0uz, "Should be empty after Clear()");
			};
		});

	SuiteRegistrar _(SUITE);
}
