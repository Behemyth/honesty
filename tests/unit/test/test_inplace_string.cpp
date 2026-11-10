import std;
import synodic.honesty.test;
import synodic.periapsis;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"inplace_string",
		[]() -> Generator
		{
			co_yield "construction"_test = [](const Requirements& requirements)
			{
				{
					peri::InplaceString str('t', 'e', 's', 't');

					requirements.ExpectEquals(str.size(), std::size_t{4});
					requirements.ExpectEquals(str.view(), "test");
				}
				{
					peri::InplaceString str("test");
					requirements.ExpectEquals(str.size(), std::size_t{4});
					requirements.ExpectEquals(str.view(), "test");
				}
				{
					peri::InplaceString<5> str(std::string_view("test"));
					requirements.ExpectEquals(str.size(), std::size_t{4});
					requirements.ExpectEquals(str.view(), "test");
				}
			};

			co_yield "assignment"_test = [](const Requirements& requirements)
			{
				peri::InplaceString<4> str("bad");

				str = "test";

				requirements.ExpectEquals(str.size(), std::size_t{4});
				requirements.ExpectEquals(str.view(), "test");
			};
		});
	SuiteRegistrar _(SUITE);
}
