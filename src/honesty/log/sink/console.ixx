export module synodic.honesty.log:sink.console;

import std;
import :colour;
import :sink;
import :types;

namespace honesty::log
{
	export class Console final : public Sink
	{
	public:
		constexpr Console() = default;

		void LogV([[maybe_unused]] LevelType level, const std::string_view fmt, const std::format_args args) override
		{
			std::println("{}", std::vformat(fmt, args));
		}

	private:
	};
}
