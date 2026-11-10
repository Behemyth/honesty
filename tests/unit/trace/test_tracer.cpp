import std;
import synodic.honesty.test;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	// Test the Tracer (uses global TracingEnabled)
	honesty::trace::Tracer tracer;

	Suite SUITE(
		"tracer",
		[]() -> Generator
		{
			co_yield "tracer_respects_global_config"_test = [](const Requirements& requirements)
			{
				// Test that Tracer respects the global TracingEnabled flag
				auto span = tracer.CreateSpan("test_span");

				if constexpr (honesty::trace::TracingEnabled)
				{
					// When enabled, we get a real Span
					const auto& data = span.Data();
					requirements.ExpectEquals(data.name, std::string_view{"test_span"});
					requirements.Expect(data.traceID.IsValid());
					requirements.Expect(data.spanID.IsValid());
				}
				else
				{
					// When disabled, we get NoopSpan
					static_assert(honesty::trace::TracingEnabled || std::is_same_v<decltype(span), honesty::NoopSpan>);
				}
			};

			co_yield "is_trivially_constructible"_test = [](const Requirements&)
			{
				// Tracer should be trivially constructible
				static_assert(std::is_trivially_constructible_v<honesty::trace::Tracer>);
			};

			co_yield "is_enabled_is_consteval"_test = [](const Requirements& requirements)
			{
				// Tracer::IsEnabled() should be consteval and match TracingEnabled
				static_assert(honesty::trace::Tracer::IsEnabled() == honesty::trace::TracingEnabled);
				requirements.Expect(true);
			};

			co_yield "zero_cost_size"_test = [](const Requirements& requirements)
			{
				// The tracer should have minimal size
				requirements.Expect(sizeof(honesty::trace::Tracer) <= sizeof(honesty::trace::TracerOptions) + 8);
			};

			co_yield "scoped_span"_test = [](const Requirements& requirements)
			{
				auto [span, guard] = tracer.CreateScopedSpan("scoped_test");

				if constexpr (honesty::trace::TracingEnabled)
				{
					requirements.ExpectEquals(span.Data().name, std::string_view{"scoped_test"});
				}
				else
				{
					static_assert(honesty::trace::TracingEnabled || std::is_same_v<decltype(span), honesty::NoopSpan>);
					static_assert(honesty::trace::TracingEnabled || std::is_same_v<decltype(guard), honesty::trace::NoopContextGuard>);
				}
			};
		});
	SuiteRegistrar _(SUITE);
}
