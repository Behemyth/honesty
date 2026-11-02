import std;
import synodic.honesty.test;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"test_log",
		[](Fixture& fixture) -> Generator
		{
			[[maybe_unused]] const std::stringstream& stream = fixture.AttachListener(honesty::log::LevelType::TRACE);

			// Verify that you can capture the information going into the suite logger
			co_yield "test"_test = [](const Requirements&)
			{
			};
		});
	SuiteRegistrar _(SUITE);
}
