import std;
import synodic.honesty.test;
import synodic.honesty.trace;

using namespace honesty::test;
using namespace honesty::test::literals;

namespace
{
	Suite SUITE(
		"sink",
		[]() -> Generator
		{
			co_yield "log_sink_export"_test = [](const Requirements& requirements)
			{
				std::ostringstream oss;
				honesty::trace::LogSink sink(oss);

				honesty::trace::SpanData data;
				data.name = "test_operation";
				data.startTime = honesty::trace::SpanData{}.startTime; // Use default time
				data.endTime = data.startTime + std::chrono::milliseconds(100);
				data.status = honesty::trace::SpanStatus::OK;

				std::array spans{data};
				sink.Export(spans);

				const std::string output = oss.str();
				requirements.Expect(output.contains("test_operation"));
				requirements.Expect(output.contains("OK"));
			};

			co_yield "log_sink_with_attributes"_test = [](const Requirements& requirements)
			{
				std::ostringstream oss;
				honesty::trace::LogSink sink(oss);

				honesty::trace::SpanData data;
				data.name = "attributed_operation";
				data.startTime = honesty::trace::SpanData{}.startTime;
				data.endTime = data.startTime + std::chrono::milliseconds(50);
				data.status = honesty::trace::SpanStatus::ERROR;
				data.statusMessage = "failed";
				data.AddAttribute("http.method", std::string_view{"GET"});
				data.AddAttribute("http.status", std::int64_t{200});

				std::array spans{data};
				sink.Export(spans);

				const std::string output = oss.str();
				requirements.Expect(output.contains("attributed_operation"));
				requirements.Expect(output.contains("ERROR"));
				requirements.Expect(output.contains("failed"));
				requirements.Expect(output.contains("http.method"));
			};

			co_yield "scatter_sink"_test = [](const Requirements& requirements)
			{
				std::ostringstream oss1, oss2;
				honesty::trace::LogSink sink1(oss1);
				honesty::trace::LogSink sink2(oss2);

				honesty::trace::Scatter scatter;
				scatter.AddSink(&sink1);
				scatter.AddSink(&sink2);

				requirements.ExpectEquals(scatter.Size(), std::size_t{2});

				honesty::trace::SpanData data;
				data.name = "scattered_span";
				data.startTime = honesty::trace::SpanData{}.startTime;
				data.endTime = data.startTime;
				data.status = honesty::trace::SpanStatus::OK;

				std::array spans{data};
				scatter.Export(spans);

				// Both sinks should have received the span
				requirements.Expect(oss1.str().contains("scattered_span"));
				requirements.Expect(oss2.str().contains("scattered_span"));
			};

			co_yield "fixed_scatter_sink"_test = [](const Requirements& requirements)
			{
				std::ostringstream oss1, oss2, oss3;
				honesty::trace::LogSink sink1(oss1);
				honesty::trace::LogSink sink2(oss2);
				honesty::trace::LogSink sink3(oss3);

				honesty::trace::FixedScatter<2> scatter;
				scatter.AddSink(&sink1);
				scatter.AddSink(&sink2);
				scatter.AddSink(&sink3); // Should be ignored (at capacity)

				requirements.ExpectEquals(scatter.Size(), std::size_t{2});

				honesty::trace::SpanData data;
				data.name = "fixed_scattered";
				data.startTime = honesty::trace::SpanData{}.startTime;
				data.endTime = data.startTime;

				std::array spans{data};
				scatter.Export(spans);

				// Only first two sinks should have received
				requirements.Expect(oss1.str().contains("fixed_scattered"));
				requirements.Expect(oss2.str().contains("fixed_scattered"));
				requirements.Expect(oss3.str().empty());
			};

			co_yield "scatter_remove_sink"_test = [](const Requirements& requirements)
			{
				std::ostringstream oss1, oss2;
				honesty::trace::LogSink sink1(oss1);
				honesty::trace::LogSink sink2(oss2);

				honesty::trace::Scatter scatter;
				scatter.AddSink(&sink1);
				scatter.AddSink(&sink2);
				scatter.RemoveSink(&sink1);

				requirements.ExpectEquals(scatter.Size(), std::size_t{1});

				honesty::trace::SpanData data;
				data.name = "after_remove";
				data.startTime = honesty::trace::SpanData{}.startTime;
				data.endTime = data.startTime;

				std::array spans{data};
				scatter.Export(spans);

				// Only sink2 should have received
				requirements.Expect(oss1.str().empty());
				requirements.Expect(oss2.str().contains("after_remove"));
			};

			co_yield "otel_sink_basic"_test = [](const Requirements& requirements)
			{
				// Redirect cout and cerr to suppress OTel output
				std::ostringstream nullCout, nullCerr;
				auto* originalCout = std::cout.rdbuf(nullCout.rdbuf());
				auto* originalCerr = std::cerr.rdbuf(nullCerr.rdbuf());

				// Create OTel sink and initialize with ostream exporter
				auto otelSink = std::make_unique<honesty::trace::OTelSink>();
				otelSink->InitWithOStreamExporter();

				// Create test span data
				honesty::trace::SpanData data;
				data.name = "otel_test_span";
				data.startTime = std::chrono::steady_clock::now();
				data.endTime = data.startTime + std::chrono::milliseconds(42);
				data.status = honesty::trace::SpanStatus::OK;
				data.kind = honesty::trace::SpanKind::INTERNAL;
				data.AddAttribute("test.attribute", std::string_view{"test_value"});
				data.AddAttribute("test.count", std::int64_t{123});

				// Export should not throw
				std::array spans{data};
				otelSink->Export(spans);
				otelSink->Flush();

				// Restore streams
				std::cout.rdbuf(originalCout);
				std::cerr.rdbuf(originalCerr);

				// Just verify no crash - OTel exporter output format is internal detail
				requirements.Expect(true);
			};

			co_yield "otel_sink_multiple_spans"_test = [](const Requirements& requirements)
			{
				// Redirect cout and cerr to suppress OTel output
				std::ostringstream nullCout, nullCerr;
				auto* originalCout = std::cout.rdbuf(nullCout.rdbuf());
				auto* originalCerr = std::cerr.rdbuf(nullCerr.rdbuf());

				auto otelSink = std::make_unique<honesty::trace::OTelSink>();
				otelSink->InitWithOStreamExporter();

				// Create multiple spans. A span refers to its name, so the names must outlive the spans.
				std::array<std::string, 3> names;
				std::vector<honesty::trace::SpanData> spans;

				for (int i = 0; i < 3; ++i)
				{
					names[static_cast<std::size_t>(i)] = "span_" + std::to_string(i);

					honesty::trace::SpanData data;
					data.name = names[static_cast<std::size_t>(i)];
					data.startTime = std::chrono::steady_clock::now();
					data.endTime = data.startTime + std::chrono::milliseconds(i * 10);
					data.status = honesty::trace::SpanStatus::OK;
					data.AddAttribute("index", std::int64_t{i});
					spans.push_back(std::move(data));
				}

				otelSink->Export(spans);
				otelSink->Flush();

				// Restore streams
				std::cout.rdbuf(originalCout);
				std::cerr.rdbuf(originalCerr);

				// Just verify no crash - OTel exporter output format is internal detail
				requirements.Expect(true);
			};

			co_yield "otel_global_init"_test = [](const Requirements& requirements)
			{
				// Redirect cout and cerr to suppress OTel output
				std::ostringstream nullCout, nullCerr;
				auto* originalCout = std::cout.rdbuf(nullCout.rdbuf());
				auto* originalCerr = std::cerr.rdbuf(nullCerr.rdbuf());

				// Test global initialization/cleanup functions
				honesty::trace::InitOTelTracing();
				honesty::trace::CleanupOTelTracing();

				// Restore streams
				std::cout.rdbuf(originalCout);
				std::cerr.rdbuf(originalCerr);

				requirements.Expect(true);
			};
		});
	SuiteRegistrar _(SUITE);
}
