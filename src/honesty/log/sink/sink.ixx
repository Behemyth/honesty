export module synodic.honesty.log:sink;

import :colour;
import std;
import :types;

namespace honesty::log
{

	export class Formatter
	{
	public:
		virtual ~Formatter()  = default;
		virtual void Format() = 0;

	private:
	};

	export class Sink
	{
	public:
		explicit constexpr Sink(const LevelType level = LevelType::DEFER) :
			level_(level)
		{
		}

		virtual ~Sink() = default;

		Sink(const Sink& other)				   = delete;
		Sink(Sink&& other) noexcept			   = default;
		Sink& operator=(const Sink& other)	   = delete;
		Sink& operator=(Sink&& other) noexcept = default;

		virtual void LogV(LevelType level, std::string_view fmt, std::format_args args) = 0;

		virtual void Flush()
		{
		}

		virtual void SetFormatter([[maybe_unused]] Formatter* formatter)
		{
		}

		LevelType Level() const
		{
			return level_;
		}

		void SetLevel(const LevelType level)
		{
			level_ = level;
		}

	protected:
		LevelType level_;
	};

}
