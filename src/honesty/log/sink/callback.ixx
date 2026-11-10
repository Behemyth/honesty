export module synodic.honesty.log:sink.callback;

import std;
import :colour;
import :sink.synchronized;
import :types;
import :logger;

namespace honesty::log
{
	export template<mutex Mutex>
	class Callback final : public SynchronizedSink<Mutex>
	{
	public:
		explicit Callback(std::function<void(const Message& message)> callback) :
			callback_(std::move(callback))
		{
		}

		void SynchronizedLogV(const LevelType level, const std::string_view fmt, std::format_args args) override
		{
			const Message message(level, std::vformat(fmt, args));
			callback_(message);
		}

	private:
		std::function<void(const Message& message)> callback_;
	};
}
