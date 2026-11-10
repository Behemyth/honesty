export module synodic.honesty.test.api:runner.standard;

import std;
import synodic.honesty.test;
import synodic.honesty.log;
import synodic.honesty.metric;

import function_ref;

namespace honesty::test
{
	export class StandardRunner final : public Runner
	{
	public:
		explicit StandardRunner(const log::Logger& logger) :
			Runner(logger)
		{
		}

		static consteval std::string_view Name()
		{
			return "default";
		}

		void Run(const Requirements& requirements, const std::function_ref<void(const Requirements&)> function) override
		{
			function(requirements);
		}

		Generator Run(const Requirements& requirements, const std::function_ref<Generator(const Requirements&)> function) override
		{
			return function(requirements);
		}

		Generator Run(const std::function_ref<Generator()> function) override
		{
			return function();
		}

		Generator Run(Fixture& fixture, const std::function_ref<Generator(Fixture&)> function) override
		{
			return function(fixture);
		}

		metric::Results Run(const std::function_ref<void(metric::RegressionContext&)> function) override
		{
			metric::RegressionContext context;
			function(context);
			return context.GetResults();
		}
	};
}
