import std;
import synodic.honesty.test;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"context",
		[]() -> Generator
		{
			co_yield "initial_context_invalid"_test = [](const Requirements& requirements)
			{
				auto& ctx = honesty::trace::Context::Current();
				ctx.Clear(); // Reset for test isolation

				requirements.Expect(!ctx.IsValid());
				requirements.Expect(!ctx.GetTraceID().IsValid());
				requirements.Expect(!ctx.GetSpanID().IsValid());
			};

			co_yield "context_set_and_get"_test = [](const Requirements& requirements)
			{
				auto& ctx = honesty::trace::Context::Current();
				ctx.Clear();

				honesty::trace::TraceID traceID;
				traceID.bytes[0] = 1;
				honesty::trace::SpanID spanID;
				spanID.bytes[0] = 2;

				ctx.SetCurrent(traceID, spanID);

				requirements.Expect(ctx.IsValid());
				requirements.ExpectEquals(ctx.GetTraceID().bytes[0], std::uint8_t{1});
				requirements.ExpectEquals(ctx.GetSpanID().bytes[0], std::uint8_t{2});

				ctx.Clear();
			};

			co_yield "context_guard_restores"_test = [](const Requirements& requirements)
			{
				auto& ctx = honesty::trace::Context::Current();
				ctx.Clear();

				honesty::trace::TraceID originalTraceID;
				originalTraceID.bytes[0] = 10;
				honesty::trace::SpanID originalSpanID;
				originalSpanID.bytes[0] = 20;

				ctx.SetCurrent(originalTraceID, originalSpanID);

				{
					honesty::trace::TraceID newTraceID;
					newTraceID.bytes[0] = 30;
					honesty::trace::SpanID newSpanID;
					newSpanID.bytes[0] = 40;

					honesty::trace::ContextGuard guard(newTraceID, newSpanID);

					// Inside guard, context should be updated
					requirements.ExpectEquals(ctx.GetTraceID().bytes[0], std::uint8_t{30});
					requirements.ExpectEquals(ctx.GetSpanID().bytes[0], std::uint8_t{40});
				}

				// After guard, original context should be restored
				requirements.ExpectEquals(ctx.GetTraceID().bytes[0], std::uint8_t{10});
				requirements.ExpectEquals(ctx.GetSpanID().bytes[0], std::uint8_t{20});

				ctx.Clear();
			};

			co_yield "trace_id_validity"_test = [](const Requirements& requirements)
			{
				honesty::trace::TraceID empty;
				requirements.Expect(!empty.IsValid());

				honesty::trace::TraceID valid;
				valid.bytes[7] = 1;
				requirements.Expect(valid.IsValid());
			};

			co_yield "span_id_validity"_test = [](const Requirements& requirements)
			{
				honesty::trace::SpanID empty;
				requirements.Expect(!empty.IsValid());

				honesty::trace::SpanID valid;
				valid.bytes[3] = 1;
				requirements.Expect(valid.IsValid());
			};

			co_yield "noop_context_guard"_test = [](const Requirements& requirements)
			{
				// NoopContextGuard should be constexpr constructible
				[[maybe_unused]] constexpr honesty::trace::NoopContextGuard guard;
				// If we reach here, test passes
				requirements.Expect(true);
			};
		});
	SuiteRegistrar _(SUITE);
}
