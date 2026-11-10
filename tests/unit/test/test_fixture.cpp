import std;
import synodic.honesty.test;
import synodic.honesty.log;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"fixture",
		[](Fixture& fixture) -> Generator
		{
			co_yield "temp_logger"_test = [&](const Requirements& requirements)
			{
				std::stringstream stream;
				const TempLogger logger = fixture.TempLog(stream);

				requirements.Expect(logger.HasSink());

				constexpr std::string_view message("test");
				logger.Info(message);

				requirements.ExpectEquals(stream.str(), message);
			};

			co_yield "log_listener"_test = [&](const Requirements& requirements)
			{
				const std::stringstream& stream = fixture.AttachListener(honesty::log::LevelType::TRACE);

				requirements.Expect(
					stream.view().empty(),
					"The stream should be empty because we just started listening");
				requirements.Expect(
					stream.view().empty(),
					"The previous expectation passed, so the log stream should be empty");

				const std::string exceptionMessage = "Test exception";

				requirements.ExpectThrow<std::runtime_error>(
					[&]()
					{
						throw std::runtime_error(exceptionMessage);
					});

				requirements.Expect(
					not stream.view().empty(),
					"An exception was thrown and should have been written to the log");

				requirements.Expect(
					stream.view().contains(exceptionMessage),
					"The exception message did not propagate to the logger as given");
			};
		});
	SuiteRegistrar _(SUITE);
}
