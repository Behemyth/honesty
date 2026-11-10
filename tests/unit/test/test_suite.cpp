import std;
import synodic.honesty.test;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	constexpr Suite SUITE(
		"suite",
		[](const Fixture&) -> Generator
		{
			co_yield "name"_test = [&](const Requirements&)
			{

			};
		});

	SuiteRegistrar _(SUITE);
}
