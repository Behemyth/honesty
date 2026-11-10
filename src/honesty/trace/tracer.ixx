export module synodic.honesty.trace:tracer;

import std;

import :context;
import :types;
import :span;
import zstring_view;

namespace honesty::trace
{
	/**
	 * @brief Simple ID generator using thread-local state
	 */
	class IDGenerator
	{
	public:
		static TraceID GenerateTraceID()
		{
			TraceID id;
			auto now = std::chrono::steady_clock::now().time_since_epoch().count();
			auto counter = traceCounter_++;

			// Simple ID generation - combine time and counter
			std::memcpy(id.bytes.data(), &now, sizeof(now));
			std::memcpy(id.bytes.data() + 8, &counter, sizeof(counter));

			return id;
		}

		static SpanID GenerateSpanID()
		{
			SpanID id;
			auto counter = spanCounter_++;
			auto thread = std::hash<std::thread::id>{}(std::this_thread::get_id());

			std::memcpy(id.bytes.data(), &counter, sizeof(counter));
			std::memcpy(id.bytes.data() + 4, &thread, 4);

			return id;
		}

	private:
		static inline std::atomic<std::uint64_t> traceCounter_{1};
		static inline thread_local std::uint64_t spanCounter_{1};
	};

	/**
	 * @brief Run-time options for a tracer.
	 */
	export struct TracerOptions
	{
		constexpr TracerOptions() = default;
	};

	/**
	 * @brief A tracer that uses global compile-time configuration.
	 *
	 * This tracer uses the global TracingEnabled constexpr to determine behavior.
	 * When tracing is disabled, all methods compile to nothing via `if constexpr`.
	 *
	 * Usage:
	 *   Tracer tracer;
	 *   auto span = tracer.CreateSpan("operation");  // NoopSpan when disabled
	 */
	export class Tracer
	{
	public:
		constexpr Tracer() = default;

		Tracer(const Tracer& other) = delete;
		constexpr Tracer(Tracer&& other) noexcept = default;
		Tracer& operator=(const Tracer& other) = delete;
		constexpr Tracer& operator=(Tracer&& other) noexcept = default;

		/**
		 * @brief Create a new span. Returns NoopSpan when tracing is disabled (zero cost).
		 */
		auto CreateSpan(const std::string_view name) const
		{
			if constexpr (TracingEnabled)
			{
				auto& ctx = Context::Current();

				TraceID traceID = ctx.IsValid() ? ctx.GetTraceID() : IDGenerator::GenerateTraceID();
				SpanID parentID = ctx.GetSpanID();
				SpanID spanID = IDGenerator::GenerateSpanID();

				return Span(name, traceID, spanID, parentID, nullptr);
			}
			else
			{
				return NoopSpan();
			}
		}

		/**
		 * @brief Create a span with automatic context propagation via RAII guard
		 */
		auto CreateScopedSpan(const std::string_view name) const
		{
			if constexpr (TracingEnabled)
			{
				auto span = CreateSpan(name);
				auto guard = ContextGuard(span.Data().traceID, span.Data().spanID);
				return std::pair{std::move(span), std::move(guard)};
			}
			else
			{
				return std::pair{NoopSpan(), NoopContextGuard()};
			}
		}

		/**
		 * @brief Check if tracing is enabled at compile time
		 */
		static consteval bool IsEnabled()
		{
			return TracingEnabled;
		}

	private:
		[[no_unique_address, maybe_unused]] TracerOptions options_;
	};
}
