export module synodic.honesty.log:sink.ringbuffer;

import std;
import :colour;
import :sink.synchronized;
import :types;

namespace honesty::log
{
	export template<mutex Mutex>
	class RingBuffer final : public SynchronizedSink<Mutex>
	{
	public:
		RingBuffer() = default;

		void SynchronizedLogV(
			[[maybe_unused]] LevelType level,
			[[maybe_unused]] std::string_view fmt,
			[[maybe_unused]] std::format_args args) override
		{
		}

	private:
	};
}
