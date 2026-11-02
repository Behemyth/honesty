import std;
import synodic.honesty.test;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"runner",
		[]() -> Generator
		{
			co_yield "declare_default"_test = [](const Requirements&)
			{
			};
		});
	SuiteRegistrar _(SUITE);
}
