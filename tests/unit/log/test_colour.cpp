import std;
import synodic.honesty.test;
import synodic.honesty.log;

using namespace honesty;
using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	constexpr std::uint32_t GREEN = 0x3A'5A'40; // #3A5A40 Hunter Green

	Suite COLOR_SUITE(
		"color",
		[]() -> Generator
		{
			co_yield "construction"_test = [](const Requirements& requirements)
			{
				requirements.ExpectEquals(log::Colour24(58, 90, 64), log::Colour24(GREEN));
			};
		});

	Suite STYLE_SUITE(
		"style",
		[]() -> Generator
		{
			co_yield "construction"_test = [](const Requirements& requirements)
			{
				constexpr log::Colour24 hunterGreen(58, 90, 64);
				constexpr log::TextStyle style(hunterGreen);

				constexpr auto foreground = style.Foreground();
				requirements.Assert(foreground.has_value());
				requirements.Expect(not style.Background().has_value());
				requirements.Expect(not style.AttributeMask());

				constexpr auto value = std::get<log::Colour24>(foreground.value());
				requirements.ExpectEquals(value, hunterGreen);
			};
		});

	Suite TERMINAL_SUITE(
		"terminal",
		[]() -> Generator
		{
			// Styled output is only produced when the terminal supports colour, otherwise the tests report as skipped
			const Tag tag = log::SupportsColour() ? RUN : SKIP;

			const auto formatTo = [](const Requirements& requirements)
			{
				std::string output;
				constexpr log::TextStyle style(log::Colour24(58, 90, 64));
				log::format_to(std::back_inserter(output), style, "{} = (58,90,64)", "Hunter Green");

				std::string input("Hunter Green = (58,90,64)");
				std::string expected;
				std::format_to(std::back_inserter(expected), "\x1b[38;2;058;090;064m{}\x1b[0m", input);

				requirements.ExpectEquals(output, expected);
			};

			const auto formatted = [](const Requirements& requirements)
			{
				const std::string output =
					format(log::TextStyle(log::Colour24(58, 90, 64)), "Hunter Green = (58,90,64)");

				std::string input("Hunter Green = (58,90,64)");
				const std::string expected = std::format("\x1b[38;2;058;090;064m{}\x1b[0m", input);

				requirements.ExpectEquals(output, expected);
			};

			const auto print = [](const Requirements& requirements)
			{
				// Test that format produces expected ANSI escape sequences
				constexpr log::Colour24 red(255, 0, 0);
				constexpr log::TextStyle redStyle(red);

				std::string output = format(redStyle, "error");
				std::string expected = std::format("\x1b[38;2;255;000;000m{}\x1b[0m", "error");

				requirements.ExpectEquals(output, expected);

				// Test with format arguments
				std::string output2 = format(redStyle, "value: {}", 42);
				std::string expected2 = std::format("\x1b[38;2;255;000;000m{}\x1b[0m", "value: 42");

				requirements.ExpectEquals(output2, expected2);
			};

			co_yield Test("format_to", tag, formatTo);
			co_yield Test("format", tag, formatted);
			co_yield Test("print", tag, print);
		});

	SuiteRegistrar _(STYLE_SUITE, COLOR_SUITE, TERMINAL_SUITE);
}
