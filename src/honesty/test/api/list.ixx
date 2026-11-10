export module synodic.honesty.test.api:list;

import std;

import synodic.honesty.log;
import synodic.honesty.test;
import synodic.honesty.test.backend;

import :types;
import :execute;

class ListReporter final : public honesty::test::CumulativeAdapter
{
public:
	explicit(false) ListReporter(const honesty::log::Logger& logger) :
		CumulativeAdapter(logger)
	{
	}

	~ListReporter() override = default;

	static consteval std::string_view Name()
	{
		return "list";
	}

	void Finalize(CumulativeData data) override
	{
		data_ = std::move(data);
	}

	const CumulativeData& Data() const
	{
		return data_;
	}

private:
	CumulativeData data_;
};

namespace honesty::test::api
{
	export struct ListParameters
	{
		explicit ListParameters(
			const std::string_view applicationName,
			const std::string_view header
			) :
			applicationName(applicationName),
			header(header)
		{
		}

		std::string_view applicationName;

		std::string_view header;
	};

	export struct ListResult
	{
		ListResult() = default;

		std::vector<SuiteDescription> suites;
	};

	export auto List(const ListParameters& parameters, Runner& runner, const log::Logger& logger) -> ListResult
	{
		// Register the list reporters and immediately grab it from the registration list
		{
			static ReporterRegistrar<ListReporter> listReporterRegistrar;
		}

		ListReporter reporter(logger);

		// TODO: Filter with list command
		const ExecuteParameters executeParameters(
			parameters.applicationName,
			"",
			true,
			parameters.header);

		std::array<Reporter*, 1> reporters{ &reporter };

		[[maybe_unused]] const ExecuteResult executeResult = Execute(executeParameters, runner, reporters, logger);

		ListResult result;


		const CumulativeAdapter::CumulativeData& data = reporter.Data();

		for (const auto& [name, tests]: data.suites)
		{
			auto& resultSuite = result.suites.emplace_back();
			resultSuite.name  = name;

			for (const auto& [testName, subTests]: tests)
			{
				TestDescription description;
				description.name = testName;

				resultSuite.tests.push_back(description);
			}
		}

		return result;
	}
}
