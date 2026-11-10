
export module synodic.honesty.test:runner;

import synodic.honesty.log;
import synodic.honesty.test.backend;

import std;

namespace honesty::test
{
	template<typename T>
	concept runner = requires {
		T::Name();
		{
			std::bool_constant<(T::Name(), true)>()
		} -> std::same_as<std::true_type>;
		std::derived_from<Runner, T>;
	};

	export template<runner T>
	class RunnerRegistrar final : RunnerRegistry
	{
	public:
		RunnerRegistrar()
		{
		}

		std::unique_ptr<Runner> Create(const log::Logger& logger) const override
		{
			return std::make_unique<T>(logger);
		}

		std::string_view Name() const override
		{
			return T::Name();
		}
	};
}
