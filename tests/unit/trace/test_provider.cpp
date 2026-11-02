import std;
import synodic.honesty.test;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	// Define enum for provider test
	enum class TestCategory
	{
		FIRST,
		SECOND,
		COUNT
	};

	Suite SUITE(
		"provider",
		[]() -> Generator
		{
			co_yield "provider_size"_test = [](const Requirements& requirements)
			{
				honesty::trace::Provider<TestCategory> provider;

				requirements.ExpectEquals(provider.Size(), std::size_t{2});
			};

			co_yield "provider_get_tracer"_test = [](const Requirements& requirements)
			{
				honesty::trace::Provider<TestCategory> provider;

				// Can get tracers by category
				[[maybe_unused]] const auto& first = provider.Get<TestCategory::FIRST>();
				[[maybe_unused]] const auto& second = provider.Get<TestCategory::SECOND>();

				// Both should be Tracer instances
				static_assert(std::is_same_v<std::decay_t<decltype(first)>, honesty::trace::Tracer>);
				static_assert(std::is_same_v<std::decay_t<decltype(second)>, honesty::trace::Tracer>);

				requirements.Expect(true);
			};

			co_yield "provider_is_enabled"_test = [](const Requirements& requirements)
			{
				// Provider::IsEnabled() should match global TracingEnabled
				static_assert(
					honesty::trace::Provider<TestCategory>::IsEnabled() == honesty::trace::TracingEnabled);
				requirements.Expect(true);
			};

			co_yield "provider_create_span"_test = [](const Requirements& requirements)
			{
				honesty::trace::Provider<TestCategory> provider;

				auto span = provider.Get<TestCategory::FIRST>().CreateSpan("provider_span");

				if constexpr (honesty::trace::TracingEnabled)
				{
					requirements.ExpectEquals(span.Data().name, std::string_view{"provider_span"});
					requirements.Expect(span.Data().traceID.IsValid());
				}
				else
				{
					static_assert(std::is_same_v<decltype(span), honesty::NoopSpan>);
					requirements.Expect(true);
				}
			};

			co_yield "provider_buffered_span_count"_test = [](const Requirements& requirements)
			{
				honesty::trace::Provider<TestCategory> provider;

				// Initially empty
				requirements.ExpectEquals(provider.BufferedSpanCount(), std::size_t{0});
			};
		});
	SuiteRegistrar _(SUITE);
}
