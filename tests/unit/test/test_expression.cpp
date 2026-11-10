import synodic.honesty.test;
import synodic.honesty.log;

import std;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"expectation",
		[](Fixture& fixture) -> Generator
		{
			co_yield "error_description"_test = [&](const Requirements& streamRequirements) -> Generator
			{
				const std::stringstream& stream = fixture.AttachListener(honesty::log::LevelType::TRACE);

				// Tests callable for error message
				const auto description = []() -> std::string
				{
					return "ERROR";
				};

				co_yield FAIL / "assert_fail"_test = [&](const Requirements& requirements)
				{
					requirements.Assert(false, "ERROR");
					requirements.Assert(false, description);
				};

				co_yield "assert_success"_test = [&](const Requirements& requirements)
				{
					requirements.Assert(true, "ERROR");
					requirements.Assert(true, description);
				};

				co_yield FAIL / "expect_fail"_test = [&](const Requirements& requirements)
				{
					requirements.Expect(false, "ERROR");
					requirements.Expect(false, description);
				};

				co_yield "assert_success"_test = [&](const Requirements& requirements)
				{
					requirements.Expect(true, "ERROR");
					requirements.Expect(true, description);
				};

				// Check that the error description "ERROR" is not present in the stream
				// (successful assertions shouldn't output their error descriptions)
				// Note: We check for absence of "ERROR" rather than empty stream to allow
				// framework diagnostic logs at higher verbosity levels
				streamRequirements.Expect(
					stream.view().find("ERROR") == std::string_view::npos,
					"Error description should not be present for successful assertions.");
			};
		});

	SuiteRegistrar _(SUITE);
}
