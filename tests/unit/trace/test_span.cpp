import std;
import synodic.honesty.test;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	// Use non-viral Tracer (uses global TracingEnabled)
	honesty::trace::Tracer tracer;

	Suite SUITE(
		"span",
		[]() -> Generator
		{
			co_yield "span_creation"_test = [](const Requirements& requirements)
			{
				if constexpr (honesty::trace::TracingEnabled)
				{
					honesty::Span span = tracer.CreateSpan("test_span");
					requirements.Expect(!span.IsEnded());
				}
				else
				{
					// When disabled, we get NoopSpan
					auto span = tracer.CreateSpan("test_span");
					static_assert(honesty::trace::TracingEnabled || std::is_same_v<decltype(span), honesty::NoopSpan>);
					requirements.Expect(true);
				}
			};

			co_yield "span_attributes"_test = [](const Requirements& requirements)
			{
				if constexpr (honesty::trace::TracingEnabled)
				{
					auto span = tracer.CreateSpan("attributed_span");

					span.SetAttribute("key1", std::int64_t{42});
					span.SetAttribute("key2", std::string_view{"value"});
					span.SetAttribute("key3", true);

					const auto& data = span.Data();
					requirements.ExpectEquals(data.attributeCount, std::size_t{3});
				}
				else
				{
					// When disabled, SetAttribute compiles to nothing
					auto span = tracer.CreateSpan("attributed_span");
					span.SetAttribute("key", std::int64_t{42});
					requirements.Expect(true);
				}
			};

			co_yield "span_status"_test = [](const Requirements& requirements)
			{
				if constexpr (honesty::trace::TracingEnabled)
				{
					auto span = tracer.CreateSpan("status_span");

					span.SetStatus(honesty::trace::SpanStatus::OK);
					requirements.ExpectEquals(
						static_cast<int>(span.Data().status),
						static_cast<int>(honesty::trace::SpanStatus::OK));

					span.SetStatus(honesty::trace::SpanStatus::ERROR, "something went wrong");
					requirements.ExpectEquals(
						static_cast<int>(span.Data().status),
						static_cast<int>(honesty::trace::SpanStatus::ERROR));
				}
				else
				{
					// When disabled, SetStatus compiles to nothing
					auto span = tracer.CreateSpan("status_span");
					span.SetStatus(honesty::trace::SpanStatus::OK);
					requirements.Expect(true);
				}
			};

			co_yield "noop_span"_test = [](const Requirements& requirements)
			{
				// NoopSpan should be constexpr constructible and have no-op methods
				constexpr honesty::NoopSpan noop;
				noop.SetStatus(honesty::trace::SpanStatus::OK);
				noop.SetAttribute("key", std::int64_t{42});
				noop.SetName("ignored");
				// If we reach here without compile errors, the test passes
				requirements.Expect(true);
			};
		});
	SuiteRegistrar _(SUITE);
}
