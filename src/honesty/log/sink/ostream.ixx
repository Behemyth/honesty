export module synodic.honesty.log:sink.ostream;

import std;
import :colour;
import :sink.synchronized;
import :types;

namespace honesty::log
{
	export template<mutex Mutex>
	class OStream final : public SynchronizedSink<Mutex>
	{
	public:
		explicit(false) OStream(std::ostream& stream) :
			stream_(stream)
		{
		}

		void SynchronizedLogV([[maybe_unused]] LevelType level, const std::string_view fmt, const std::format_args args) override
		{
			stream_.get() << std::vformat(fmt, args);
		}

		void SynchronizedFlush() override
		{
			stream_.get().flush();
		}

	private:
		std::reference_wrapper<std::ostream> stream_;
	};
}
