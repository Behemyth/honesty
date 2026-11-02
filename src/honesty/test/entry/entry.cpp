import std;

import synodic.honesty.log;
import synodic.honesty.test.api;
import synodic.honesty.test;

auto main(const int argc, const char* argv[]) -> int
{
	// Convert all the input arguments to views
	auto view = std::span(argv, argc) | std::views::transform(
		            [](const char* value) -> std::string_view
		            {
			            // strlen - linear search under the hood
			            return std::string_view(value);
		            });

	// Convert the lazy view to contiguous data
	auto arguments = std::ranges::to<std::vector>(view);

	honesty::log::Console sink;

	honesty::log::Logger logger = honesty::log::RootLogger().CreateLogger("main");

	logger.SetSink(&sink);
	logger.SetLevel(honesty::log::LevelType::INFO);

	honesty::test::StandardRunner runner(logger);

	// 'list' prints one 'suite.test' name per line, the format consumed by CMake's discover_tests
	if (arguments.size() == 2 and arguments[1] == "list")
	{
		const honesty::test::api::ListParameters parameters("honesty", {});
		const honesty::test::api::ListResult result = List(parameters, runner, logger);

		std::vector<std::string> names;
		for (const auto& suite: result.suites)
		{
			for (const auto& test: suite.tests)
			{
				std::string name = std::format("{}.{}", suite.name, test.name);
				if (not std::ranges::contains(names, name))
				{
					std::println("{}", name);
					names.push_back(std::move(name));
				}
			}
		}

		return 0;
	}

	std::string_view filter;

	if (arguments.size() == 3 and arguments[1] == "--filter")
	{
		filter = arguments[2];
	}
	else if (arguments.size() != 1)
	{
		std::println(std::cerr, "usage: {} [list | --filter <suite>[.<test>...]]", arguments.front());
		return 2;
	}

	honesty::test::ReporterRegistrar<honesty::test::StandardReporter> standardReporter;

	const honesty::test::api::ExecuteParameters
		configuration("honesty", filter, false, {});

	const honesty::test::api::ExecuteResult result = Execute(configuration, runner, {}, logger);

	return result.success ? 0 : 1;
}
