export module synodic.honesty.trace:span;

import std;

import synodic.honesty.metric;

import :context;
import :scope;
import :types;

namespace honesty::trace
{
	export class Sink;

	/**
	 * @brief The collected data of a span. Meant to be a small size and efficient to process in bulk numbers
	 */
	export struct SpanData
	{
		std::string_view name;
		honesty::metric::Clock::time_point startTime;
		honesty::metric::Clock::time_point endTime;
		TraceID traceID;
		SpanID spanID;
		SpanID parentSpanID;
		SpanStatus status = SpanStatus::UNSET;
		SpanKind kind = SpanKind::INTERNAL;
		std::string statusMessage;

		// Small buffer optimization for attributes - most spans have few attributes
		static constexpr std::size_t INLINE_ATTRIBUTES = 8;
		std::array<Attribute, INLINE_ATTRIBUTES> inlineAttributes{
			Attribute{"", false}, Attribute{"", false}, Attribute{"", false}, Attribute{"", false},
			Attribute{"", false}, Attribute{"", false}, Attribute{"", false}, Attribute{"", false}
		};
		std::size_t attributeCount = 0;

		constexpr void AddAttribute(std::string_view key, AttributeValue value)
		{
			if (attributeCount < INLINE_ATTRIBUTES)
			{
				inlineAttributes[attributeCount++] = Attribute{key, std::move(value)};
			}
			// TODO: Overflow to heap allocation if needed
		}

		constexpr std::span<const Attribute> Attributes() const
		{
			return std::span{inlineAttributes.data(), attributeCount};
		}
	};

	/**
     * @brief A no-op span for when tracing is disabled. Compiles to nothing.
     */
	export class NoopSpan
	{
	public:
		constexpr explicit NoopSpan() noexcept = default;

		// No-op methods that compile away
		constexpr void SetStatus(SpanStatus /*status*/, std::string_view /*message*/ = {}) const {}
		constexpr void SetAttribute(std::string_view /*key*/, AttributeValue /*value*/) const {}
		constexpr void SetName(std::string_view /*name*/) const {}
	};

	/**
	 * @brief The public interface for recording a span of time
	 */
	export class Span
	{
		friend class Tracer;

	public:
		~Span()
		{
			End();
		}

		Span(const Span&) = delete;
		Span& operator=(const Span&) = delete;
		Span(Span&& other) noexcept :
			data_(std::move(other.data_)),
			sink_(other.sink_),
			ended_(other.ended_)
		{
			other.ended_ = true; // Prevent double-end
		}
		Span& operator=(Span&& other) noexcept
		{
			if (this != &other)
			{
				End();
				data_ = std::move(other.data_);
				sink_ = other.sink_;
				ended_ = other.ended_;
				other.ended_ = true;
			}
			return *this;
		}

		void SetStatus(SpanStatus status, std::string_view message = {})
		{
			data_.status = status;
			data_.statusMessage = message;
		}

		void SetAttribute(std::string_view key, AttributeValue value)
		{
			data_.AddAttribute(key, std::move(value));
		}

		void SetName(std::string_view name)
		{
			data_.name = name;
		}

		void End()
		{
			if (!ended_)
			{
				data_.endTime = metric::Clock::now();
				ended_ = true;
				// Span data will be exported by the tracer/provider
			}
		}

		const SpanData& Data() const { return data_; }
		bool IsEnded() const { return ended_; }

	private:
		explicit Span(std::string_view name, TraceID traceID, SpanID spanID, SpanID parentID, Sink* sink) :
			sink_(sink)
		{
			data_.name = name;
			data_.startTime = metric::Clock::now();
			data_.traceID = traceID;
			data_.spanID = spanID;
			data_.parentSpanID = parentID;
		}

		SpanData data_;
		Sink* sink_ = nullptr;
		bool ended_ = false;
	};
}
