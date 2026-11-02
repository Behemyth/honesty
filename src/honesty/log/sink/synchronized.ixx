export module synodic.honesty.log:sink.synchronized;

import :colour;
import std;
import :types;
import :sink;

namespace honesty::log
{
	export template<mutex Mutex>
	class SynchronizedSink : public Sink
	{
	public:
		explicit SynchronizedSink(LevelType level = LevelType::DEFER) :
			Sink(level)
		{
		}

		~SynchronizedSink() override = default;

		void LogV(LevelType level, std::string_view fmt, std::format_args args) final
		{
			std::lock_guard<Mutex> lock(mutex_);
			SynchronizedLogV(level, fmt, args);
		}

		void Flush() final
		{
			std::lock_guard<Mutex> lock(mutex_);
			SynchronizedFlush();
		}

		void SetFormatter(Formatter* formatter) final
		{
			std::lock_guard<Mutex> lock(mutex_);
			SynchronizedSetFormatter(formatter);
		}

	protected:
		virtual void SynchronizedLogV(LevelType level, std::string_view fmt, std::format_args args) = 0;

		virtual void SynchronizedFlush()
		{
		}

		virtual void SynchronizedSetFormatter([[maybe_unused]] Formatter* formatter)
		{
		}

		Mutex mutex_;
	};
}
