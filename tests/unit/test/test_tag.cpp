import std;
import synodic.honesty.test;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"tag",
		[]() -> Generator
		{
			co_yield "traits"_test = [](const Requirements& requirements)
			{
				// We want the tag size to fit within a nice, small size
				constexpr int size = sizeof(Tag);
				requirements.ExpectLessEqual(size, 128);
			};

			// TODO: Do we support error descriptions on tags?

			co_yield "construction"_test = [](const Requirements& requirements)
			{
				{
					constexpr Tag tag('t', 'e', 's', 't');

					requirements.Expect(tag.Size());

					constexpr auto data = tag.View()[0];

					requirements.Expect(data == "test");
				}

				{
					constexpr Tag tag("test");

					requirements.Expect(tag.Size());

					constexpr auto data = tag.View()[0];

					requirements.Expect(data == "test");
				}

				{
					constexpr Tag tag(std::string_view("test"));

					requirements.Expect(tag.Size());

					constexpr auto data = tag.View()[0];

					requirements.Expect(data == "test");
				}
			};

			co_yield "range"_test = [](const Requirements& requirements)
			{
				for (constexpr Tag tags("test"); std::string_view tag: tags)
				{
					requirements.Expect(not tag.empty());
				}
			};

			co_yield "equality"_test = [](const Requirements& requirements)
			{
				// Test single tag equality
				{
					constexpr auto tag = SKIP;

					requirements.Expect(tag == SKIP);
					requirements.Expect(tag == "skip");
					requirements.Expect(tag != "run");
				}

				// Test combined tag equality (Tag / Tag)
				{
					constexpr auto tag = Tag("test") / Tag("skip");

					requirements.Expect(tag == SKIP);
					requirements.Expect(tag == "test");
					requirements.Expect(tag != "run");
				}

				// Test combined tag equality (Tag / predefined)
				{
					constexpr auto tag = Tag("test") / SKIP;

					requirements.Expect(tag == SKIP);
					requirements.Expect(tag == "test");
					requirements.Expect(tag != "run");
				}

				// Test combined tag equality (predefined / Tag)
				{
					constexpr auto tag = SKIP / Tag("test");

					requirements.Expect(tag == SKIP);
					requirements.Expect(tag == "test");
					requirements.Expect(tag != "run");
				}

				// Test duplicate tag combination
				{
					constexpr auto tag = SKIP / SKIP;

					requirements.Expect(tag == SKIP);
					requirements.Expect(tag != "test");
					requirements.Expect(tag != "run");
				}
			};

			co_yield "test"_test = [](const Requirements& requirements)
			{
				{
					const Test test = TODO / "test"_tag / "inner"_test = [](const Requirements&)
					{
					};
					requirements.ExpectEquals(test.Tags().size(), std::size_t{2});
				}
			};

			co_yield FAIL / "fail"_test = [](const Requirements& requirements)
			{
				requirements.Expect(false);
			};

			co_yield "skip"_test = [](const Requirements& requirements)
			{
				// Verify SKIP tag has correct value
				requirements.Expect(SKIP == "skip");
				requirements.Expect(SKIP.Size() == 1);

				// Verify combining with SKIP preserves the skip tag
				constexpr auto combined = Tag("mytest") / SKIP;
				requirements.Expect(combined == SKIP);
				requirements.Expect(combined == "mytest");
				requirements.ExpectEquals(combined.Size(), std::size_t{2});
			};

			co_yield "todo"_test = [](const Requirements& requirements)
			{
				// Verify TODO tag has correct value
				requirements.Expect(TODO == "todo");
				requirements.Expect(TODO.Size() == 1);

				// Verify combining with TODO preserves the todo tag
				constexpr auto combined = Tag("incomplete") / TODO;
				requirements.Expect(combined == TODO);
				requirements.Expect(combined == "incomplete");
				requirements.ExpectEquals(combined.Size(), std::size_t{2});
			};
		});
	SuiteRegistrar _(SUITE);
}
