export module synodic.honesty.test.api:reporter.standard;

import synodic.honesty.log;
import synodic.honesty.test;
import synodic.honesty.metric;
import synodic.periapsis;
import std;

namespace
{
	constexpr auto HIGHLIGHT_STYLE(honesty::log::TextStyle(honesty::log::Colour24(255, 255, 0)));
	constexpr auto SUCCESS_STYLE(honesty::log::TextStyle(honesty::log::Colour24(0, 255, 0)));
	constexpr auto FAILURE_STYLE(honesty::log::TextStyle(honesty::log::Colour24(255, 0, 0)));
	constexpr auto WARNING_STYLE(honesty::log::TextStyle(honesty::log::Colour24(255, 200, 0)));
	constexpr auto SKIP_STYLE(honesty::log::TextStyle(honesty::log::Colour24(128, 128, 128)));
	constexpr auto BENCHMARK_STYLE(honesty::log::TextStyle(honesty::log::Colour24(0, 200, 200)));
	constexpr auto BENCHMARK_NAME_STYLE(honesty::log::TextStyle(honesty::log::Colour24(200, 150, 255)));
}

namespace honesty::test
{
	/**
	 * @brief The state required for the lifetime of a test
	 */
	class TestState
	{
	public:
		struct Output
		{
			std::size_t passedAssertions          = 0;
			std::size_t failedAssertions          = 0;
			std::size_t undefinedPassedAssertions = 0;
			std::size_t undefinedFailedAssertions = 0;
		};

		TestState(const event::TestBegin& event, const log::Logger& logger) :
			testOutcome_(event.outcome),
			logger_(logger),
			name_(event.name)
		{
		}

		void Signal(const event::AssertionPass& event)
		{
			ReportSuccess(event);
		}

		void Signal(const event::AssertionFail& event)
		{
			ReportFailure(event);
		}

		void Signal(const event::EqualityFail& event)
		{
			ReportFailure(event);

			const std::string_view relation = event.equal ? "==" : "!=";

			ReportFailureSummary(relation, event.a, event.b);
		}

		void Signal(const event::ComparisonFail& event)
		{
			ReportFailure(event);

			const std::string_view relation = [](const std::strong_ordering ordering) -> std::string_view
			{
				if (ordering == std::strong_ordering::less)
				{
					return "<";
				}
				if (ordering == std::strong_ordering::greater)
				{
					return ">";
				}
				return "==";
			}(event.ordering);

			ReportFailureSummary(relation, event.a, event.b);
		}

		Output Summarize(const event::TestEnd&)
		{
			return output_;
		}

		std::string_view Name() const
		{
			return name_;
		}

	private:
		/**
		 * @brief Provides consistent text styling for assertion failure summaries.
		 * @param relation The string representing the relationship between the two failed comparisons.
		 * @param a Comparison value a.
		 * @param b Comparison value b.
		 */
		void ReportFailureSummary(std::string_view relation, std::string_view a, std::string_view b) const
		{
			constexpr std::size_t maxLineLength = 79;

			const log::Logger& logger = logger_.get();
			constexpr std::string_view indent("  ");

			// Size of the inputs including the space between them.
			const std::size_t totalSize = a.size() + b.size() + 2 + relation.size();

			// Check to see if the compacted summary can fit on a single line
			if (a.contains("\n") or b.contains("\n") or totalSize >= maxLineLength)
			{
				throw peri::NotImplementedException();
			}

			const std::string introduction = format("{}Expected:", indent);
			const std::string expected     = format(HIGHLIGHT_STYLE, "{}{}'{}'", indent, indent, a);
			const std::string relationText = format("{}{}{}{}", indent, indent, indent, relation);
			const std::string actual       = format(HIGHLIGHT_STYLE, "{}{}'{}'", indent, indent, b);

			logger.Info("{}\n{}\n{}\n{}", introduction, expected, relationText, actual);
		}

		/**
		 * @brief Reports the success of an assertion.
		 * @param event The assertion event.
		 */
		void ReportSuccess(const event::AssertionPass& event)
		{
			const log::Logger& logger = logger_.get();
			switch (testOutcome_)
			{
				case ExpectedAssertOutcome::PASS:
				{
					++output_.passedAssertions;
					break;
				}
				case ExpectedAssertOutcome::FAIL:
				{
					std::string styledResult = format(FAILURE_STYLE, "Failed");
					logger.Info(
						"Test {}: File({}), Line({})",
						styledResult,
						event.location.file_name(),
						event.location.line());

					++output_.failedAssertions;
					break;
				}
				case ExpectedAssertOutcome::TODO:
				{
					++output_.undefinedPassedAssertions;
					break;
				}
			}
		}

		/**
		 * @brief Reports the failure of an assertion.
		 * @param event The assertion event.
		 */
		void ReportFailure(const event::AssertionFail& event)
		{
			const log::Logger& logger = logger_.get();
			switch (testOutcome_)
			{
				case ExpectedAssertOutcome::PASS:
				{
					std::string styledResult = format(FAILURE_STYLE, "Failed");
					logger.Info(
						"Test {}: File({}), Line({})",
						styledResult,
						event.location.file_name(),
						event.location.line());

					if (!event.message.empty())
					{
						constexpr std::string_view indent("  ");
						logger.Info("{}Description: {}", indent, event.message);
					}

					++output_.failedAssertions;

					break;
				}
				case ExpectedAssertOutcome::FAIL:
				{
					++output_.passedAssertions;

					break;
				}
				case ExpectedAssertOutcome::TODO:
				{
					++output_.undefinedFailedAssertions;
					break;
				}
			}
		}

		ExpectedAssertOutcome testOutcome_;
		std::reference_wrapper<const log::Logger> logger_;

		std::string_view name_;

		Output output_;
	};

	class SuiteState
	{
	public:
		struct Output
		{
			std::size_t passedAssertions          = 0;
			std::size_t failedAssertions          = 0;
			std::size_t undefinedPassedAssertions = 0;
			std::size_t undefinedFailedAssertions = 0;

			std::size_t skippedTests = 0;
			std::size_t todoTests    = 0;
		};

		SuiteState(const event::SuiteBegin& event, const log::Logger& logger) :
			logger_(logger),
			name_(event.name)
		{
		}

		void Signal(const event::TestBegin& event)
		{
			testStates_.emplace_back(event, logger_);
		}

		void Signal(const event::TestEnd& event)
		{
			TestState& state = testStates_.back();
			const auto [passedAssertions, failedAssertions, undefinedPassedAssertions, undefinedFailedAssertions] =
				state.Summarize(event);

			output_.passedAssertions += passedAssertions;
			output_.failedAssertions += failedAssertions;
			output_.undefinedPassedAssertions += undefinedPassedAssertions;
			output_.undefinedFailedAssertions += undefinedFailedAssertions;

			testStates_.pop_back();
		}

		void Signal(const event::TestSkip& event)
		{
			if (event.todo)
			{
				const log::Logger& logger = logger_.get();

				std::string name(name_);

				for (auto& test: testStates_)
				{
					name.append(".");
					name.append(test.Name());
				}

				std::string styledToDo = format(HIGHLIGHT_STYLE, "TODO");
				std::string styledName = format(HIGHLIGHT_STYLE, "{}", name);

				logger.Info("{}: Implement '{}' test", styledToDo, styledName);

				++output_.todoTests;
			}
			else
			{
				++output_.skippedTests;
			}
		}

		void Signal(const event::AssertionPass& event)
		{
			TestState& state = testStates_.back();
			state.Signal(event);
		}

		void Signal(const event::AssertionFail& event)
		{
			TestState& state = testStates_.back();
			state.Signal(event);
		}

		void Signal(const event::EqualityFail& event)
		{
			TestState& state = testStates_.back();
			state.Signal(event);
		}

		void Signal(const event::ComparisonFail& event)
		{
			TestState& state = testStates_.back();
			state.Signal(event);
		}

		Output Summarize(const event::SuiteEnd&)
		{
			return output_;
		}

	private:
		// Tests are recursive, and we should keep track of state as such.
		std::list<TestState> testStates_;

		std::reference_wrapper<const log::Logger> logger_;

		std::string_view name_;

		Output output_;
	};

	export class StandardReporter final : public StreamingAdapter
	{
	public:
		explicit StandardReporter(const log::Logger& logger) :
			StreamingAdapter(logger),
			history_(metric::HistoryConfig {})
		{
		}

		~StandardReporter() override = default;

		static consteval std::string_view Name()
		{
			return "default";
		}

		void Signal(const event::Initialize& event) override
		{
			const log::Logger& logger = Logger();

			logger.Info("{}\nSeed: {}", event.header, event.seed);
			logger.Info("-------------------");
		}

		void Signal(const event::SuiteBegin& event) override
		{
			currentSuiteState_.emplace(event, Logger());
		}

		void Signal(const event::SuiteEnd& event) override
		{
			SuiteState& state = currentSuiteState_.value();
			const auto
			[passedAssertions,
				failedAssertions,
				undefinedPassedAssertions,
				undefinedFailedAssertions,
				skippedTests,
				todoTests] = state.Summarize(event);

			assertionsPassedCount_ += passedAssertions;
			assertionFailedCount_ += failedAssertions;
			skippedTestsCount_ += skippedTests;
			todoTestsCount_ += todoTests;

			currentSuiteState_.reset();
		}

		void Signal(const event::TestBegin& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::TestEnd& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::TestSkip& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::AssertionPass& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::AssertionFail& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::EqualityFail& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::ComparisonFail& event) override
		{
			currentSuiteState_->Signal(event);
		}

		void Signal(const event::Summary& event) override
		{
			const log::Logger& logger = Logger();

			const auto totalSeconds = std::chrono::duration<double>(event.duration);

			std::string passedStyled = format(SUCCESS_STYLE, "Passed");
			std::string failedStyled = format(FAILURE_STYLE, "Failed");
			std::string skipStyled   = format(SKIP_STYLE, "Skipped");
			std::string todoStyled   = format(HIGHLIGHT_STYLE, "TODO");

			logger.Info("{} Assertions {} ({:.3f}s)", assertionsPassedCount_, passedStyled, totalSeconds.count());

			if (assertionFailedCount_)
			{
				logger.Info("{} Assertions {}", assertionFailedCount_, failedStyled);
			}

			if (skippedTestsCount_)
			{
				logger.Info("{} Tests {}", skippedTestsCount_, skipStyled);
			}

			if (todoTestsCount_)
			{
				logger.Info("{} Tests Marked {}", todoTestsCount_, todoStyled);
			}
		}

		void Signal(const event::BenchmarkComplete& event) override
		{
			const log::Logger& logger = Logger();
			const std::string benchmarkName(event.name);

			// Convert mean to nanoseconds to preserve precision for fast operations
			const auto meanNs = std::chrono::duration_cast<std::chrono::nanoseconds>(event.results.mean);
			const auto stddevNs = std::chrono::duration_cast<std::chrono::nanoseconds>(event.results.stddev);

			std::string benchmarkStyled = format(BENCHMARK_STYLE, "Benchmark");
			std::string nameStyled = format(BENCHMARK_NAME_STYLE, "{}", event.name);

			logger.Info(
				"{}: {}",
				benchmarkStyled,
				nameStyled);
			logger.Info(
				"  {} ns +/- {} ns ({} iterations, {:.2f} ops/s)",
				meanNs.count(),
				stddevNs.count(),
				event.results.iterations,
				event.results.iterations_per_second);

			// Compare against history and display changes
			if (auto comparison = history_.Compare(benchmarkName, event.results))
			{
				DisplayHistoryComparison(*comparison, logger);
			}

			// Record this result for future comparisons
			history_.Record(benchmarkName, event.results);
		}

		void Signal(const event::BenchmarkGroupComplete& event) override
		{
			const log::Logger& logger = Logger();

			if (event.entries.empty())
			{
				logger.Info("Benchmark Group: {} (empty)", event.groupName);
				return;
			}

			// Find baseline (first one if multiple)
			const event::BenchmarkEntry* baseline = nullptr;
			bool multipleBaselines = false;

			for (const auto& entry: event.entries)
			{
				if (entry.isBaseline)
				{
					if (baseline != nullptr)
					{
						multipleBaselines = true;
					}
					else
					{
						baseline = &entry;
					}
				}
			}

			// Calculate max name width for alignment
			std::size_t maxNameWidth = 0;
			for (const auto& entry: event.entries)
			{
				maxNameWidth = std::max(maxNameWidth, entry.name.size());
			}

			std::string benchmarkStyled = format(BENCHMARK_STYLE, "Benchmark");
			std::string groupStyled = format(BENCHMARK_NAME_STYLE, "{}", event.groupName);
			logger.Info("{}: {}", benchmarkStyled, groupStyled);

			for (const auto& entry: event.entries)
			{
				const auto meanNs = std::chrono::duration_cast<std::chrono::nanoseconds>(entry.results.mean);
				const auto stddevNs = std::chrono::duration_cast<std::chrono::nanoseconds>(entry.results.stddev);

				// Build comparison string
				std::string comparison;
				if (entry.isBaseline)
				{
					if (&entry == baseline)
					{
						comparison = "baseline";
					}
					else
					{
						comparison = format(WARNING_STYLE, "(ignored baseline)");
					}
				}
				else if (baseline != nullptr)
				{
					const auto baselineMean = baseline->results.mean.count();
					const auto entryMean = entry.results.mean.count();

					if (entryMean > 0)
					{
						// Calculate speedup ratio as recommended by Bruce Dawson:
						// https://randomascii.wordpress.com/2018/02/04/what-we-talk-about-when-we-talk-about-performance/
						// speedup ratio = OldTime / NewTime (for times) = baseline / entry
						// Use "X times as fast" phrasing to avoid ambiguous percentages
						const double speedupRatio = baselineMean / entryMean;
						comparison = std::format("{:.2f}x", speedupRatio);
					}
				}

				// Pad name to max width, then style it (so ANSI codes don't affect alignment)
				std::string paddedName = std::format("{:<{}}", entry.name, maxNameWidth);
				std::string nameStyled = format(BENCHMARK_NAME_STYLE, "{}", paddedName);

				// Build history comparison string (inline)
				const std::string fullName = event.groupName + "/" + entry.name;
				std::string historyText;
				if (auto histComparison = history_.Compare(fullName, entry.results))
				{
					historyText = FormatHistoryInline(*histComparison);
				}

				// Output with alignment: name, mean +/- stddev, comparison, history
				if (historyText.empty())
				{
					logger.Info(
						"  {}   {:>8} ns +/- {:>8} ns   {}",
						nameStyled,
						meanNs.count(),
						stddevNs.count(),
						comparison);
				}
				else
				{
					logger.Info(
						"  {}   {:>8} ns +/- {:>8} ns   {}  {}",
						nameStyled,
						meanNs.count(),
						stddevNs.count(),
						comparison,
						historyText);
				}

				// Record for future comparisons
				history_.Record(fullName, entry.results);
			}

			if (multipleBaselines)
			{
				std::string warningStyled = format(WARNING_STYLE, "Warning");
				logger.Warning("  {}: Multiple baselines detected, using first one", warningStyled);
			}
		}

	private:
		/**
		 * @brief Format history comparison for inline display using speedup ratios
		 *
		 * Following Bruce Dawson's recommendation to use ratios instead of percentages:
		 * https://randomascii.wordpress.com/2018/02/04/what-we-talk-about-when-we-talk-about-performance/
		 * speedup ratio = OldTime / NewTime
		 * > 1.0 means faster, < 1.0 means slower
		 */
		std::string FormatHistoryInline(const metric::ComparisonResult& comparison) const
		{
			// Calculate speedup ratio: previous / current
			// > 1.0 means current is faster, < 1.0 means current is slower
			const double speedupRatio = comparison.previous.meanNs / comparison.current.meanNs;

			switch (comparison.status)
			{
				case metric::ComparisonResult::Status::Improved:
					return format(SUCCESS_STYLE, "[{:.2f}x vs prev]", speedupRatio);
				case metric::ComparisonResult::Status::Regressed:
					return format(FAILURE_STYLE, "[{:.2f}x vs prev]", speedupRatio);
				case metric::ComparisonResult::Status::Unchanged:
				default:
					return format(SKIP_STYLE, "[{:.2f}x vs prev]", speedupRatio);
			}
		}

		/**
		 * @brief Display history comparison for a single benchmark using speedup ratios
		 */
		void DisplayHistoryComparison(const metric::ComparisonResult& comparison, const log::Logger& logger) const
		{
			const auto& prev = comparison.previous;

			// Calculate speedup ratio: previous / current
			const double speedupRatio = prev.meanNs / comparison.current.meanNs;
			std::string prevText = metric::FormatDuration(prev.meanNs);

			std::string changeText;
			switch (comparison.status)
			{
				case metric::ComparisonResult::Status::Improved:
					changeText = format(SUCCESS_STYLE, "{:.2f}x as fast", speedupRatio);
					break;
				case metric::ComparisonResult::Status::Regressed:
					changeText = format(FAILURE_STYLE, "{:.2f}x as fast", speedupRatio);
					break;
				case metric::ComparisonResult::Status::Unchanged:
				default:
					changeText = format(SKIP_STYLE, "{:.2f}x", speedupRatio);
					break;
			}

			logger.Info("  vs previous: {} (was {})", changeText, prevText);
		}

		std::size_t assertionsPassedCount_ = 0;
		std::size_t assertionFailedCount_  = 0;
		std::size_t skippedTestsCount_     = 0;
		std::size_t todoTestsCount_        = 0;

		std::optional<SuiteState> currentSuiteState_;
		metric::BenchmarkHistory history_;
	};
}
