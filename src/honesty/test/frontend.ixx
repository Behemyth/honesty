export module synodic.honesty.test:test;

import std;

import synodic.honesty.test.backend;
import synodic.honesty.metric;

import function_ref;

namespace honesty::test
{
	// An explicit tage for the default test state.
	export constexpr Tag RUN("run");

	// Skips a test, recording it as skipped
	export constexpr Tag SKIP("skip");

	// Marks a test as expected to fail, records a failure if it passes or partially passes
	export constexpr Tag FAIL("fail");

	// Registers the test as incomplete, recording it as such
	export constexpr Tag TODO("todo");

	// Runs the test silently without reporter output or counting in totals.
	// Useful for meta-tests that test the framework itself.
	export constexpr Tag SILENT("silent");

	// Marks a benchmark as the baseline for comparison within a group.
	// This is a BenchmarkTag - it can only be used with benchmarks, not tests.
	export constexpr BenchmarkTag BASELINE("baseline");

	/**
	 * @brief Represents a test case without an implementation
	 */
	class TestDescriptor
	{
	public:
		explicit(false) consteval TestDescriptor(const std::string_view name) :
			name_(name)
		{
		}

		consteval TestDescriptor(const std::string_view name, const Tag& tag) :
			name_(name),
			tag_(tag)
		{
		}

		TestDescriptor(const TestDescriptor& other)                          = delete;
		consteval TestDescriptor(TestDescriptor&& other) noexcept            = default;
		TestDescriptor& operator=(const TestDescriptor& other)               = delete;
		consteval TestDescriptor& operator=(TestDescriptor&& other) noexcept = default;

		template<typename Fn>
			requires std::invocable<Fn, const Requirements&> &&
			         std::same_as<void, std::invoke_result_t<Fn, const Requirements&>>
		inline Test operator=(const Fn& test) const
		{
			return Test(name_, tag_, std::function_ref<void(const Requirements&)>(test));
		}

		/**
		 * @brief Assigns a pure generator test function to the test descriptor. You may find it useful to
		 *	use this to label multiple children tests with a common tag without setting up any additional
		 *	requirements
		 */
		template<typename Fn>
			requires std::invocable<Fn> &&
			         std::same_as<Generator, std::invoke_result_t<Fn>>
		inline Test operator=(const Fn& test) const
		{
			return Test(name_, tag_, std::function_ref<Generator()>(test));
		}

		template<typename Fn>
			requires std::invocable<Fn, const Requirements&> &&
			         std::same_as<Generator, std::invoke_result_t<Fn, const Requirements&>>
		inline Test operator=(const Fn& test) const
		{
			return Test(name_, tag_, std::function_ref<Generator(const Requirements&)>(test));
		}

		auto operator=(Generator&& generator) const
		{
			return std::ranges::elements_of(std::forward<Generator>(generator));
		}

		friend consteval TestDescriptor operator/(const Tag& tag, const TestDescriptor& test)
		{
			return TestDescriptor(test.name_, tag);
		}

	private:
		std::string_view name_;

		Tag tag_;
	};

	export template<typename Fn, std::ranges::input_range R>
		requires std::regular_invocable<Fn&, const Requirements&, std::ranges::range_reference_t<R>>
	Generator operator|(Fn&& function, R&& range)
	{
		int index = 0;
		for (auto&& element: range)
		{
			co_yield Test(
				std::format("{}", index++),
				Tag(),
				[&function, &element](const Requirements& requirements)
				{
					function(requirements, element);
				});
		}
	}

	export template<typename Fn, typename... Types>
		requires(std::regular_invocable<Fn&, const Requirements&, Types&&> && ...)
	Generator operator|(Fn&& function, std::tuple<Types...>&& tuple)
	{
		co_yield std::ranges::elements_of(
			std::apply(
				[&function](auto&&... args) -> Generator
				{
					int index = 0;
					((co_yield Test(
							std::format("{}", index++),
							Tag(),
							[&function, &args](const Requirements& requirements)
							{
								function(requirements, args);
							})),
						...);
				},
				std::forward<std::tuple<Types...>>(tuple)));
	}

	export namespace literals
	{
		[[nodiscard]] consteval auto operator""_test(const char* const name, const std::size_t size) -> TestDescriptor
		{
			return TestDescriptor(std::string_view(name, size));
		}

		[[nodiscard]] consteval auto operator""_tag(const char* const name, const std::size_t size) -> Tag
		{
			std::string_view tag(name, size);
			return Tag(std::string_view(name, size));
		}
	}

	/**
	 * @brief Represents a benchmark descriptor that produces Benchmark objects.
	 *	Can be used both at suite level (implicitly converts to Test) and inside
	 *	BenchmarkGenerator (yields Benchmark directly).
	 */
	class BenchmarkDescriptor
	{
	public:
		explicit(false) consteval BenchmarkDescriptor(const std::string_view name) :
			name_(name)
		{
		}

		consteval BenchmarkDescriptor(const std::string_view name, const Tag& tag) :
			name_(name),
			tag_(tag)
		{
		}

		BenchmarkDescriptor(const BenchmarkDescriptor& other)                          = delete;
		consteval BenchmarkDescriptor(BenchmarkDescriptor&& other) noexcept            = default;
		BenchmarkDescriptor& operator=(const BenchmarkDescriptor& other)               = delete;
		consteval BenchmarkDescriptor& operator=(BenchmarkDescriptor&& other) noexcept = default;

		/**
		 * @brief Assigns a benchmark function that receives a RegressionContext
		 */
		template<typename Fn>
			requires std::invocable<Fn, metric::RegressionContext&> &&
			         std::same_as<void, std::invoke_result_t<Fn, metric::RegressionContext&>>
		inline Benchmark operator=(const Fn& benchmark) const
		{
			return Benchmark(name_, tag_, std::function_ref<void(metric::RegressionContext&)>(benchmark));
		}

		/**
		 * @brief Assigns a generator function for benchmark grouping
		 */
		template<typename Fn>
			requires std::invocable<Fn> &&
			         std::same_as<BenchmarkGenerator, std::invoke_result_t<Fn>>
		inline Benchmark operator=(const Fn& generator) const
		{
			return Benchmark(name_, tag_, std::function_ref<BenchmarkGenerator()>(generator));
		}

		/// Tag / BenchmarkDescriptor - allows regular tags like SILENT on benchmarks
		friend consteval BenchmarkDescriptor operator/(const Tag& tag, const BenchmarkDescriptor& benchmark)
		{
			return BenchmarkDescriptor(benchmark.name_, tag / benchmark.tag_);
		}

		/// BenchmarkTag / BenchmarkDescriptor - allows benchmark-only tags like BASELINE
		friend consteval BenchmarkDescriptor operator/(const BenchmarkTag& tag, const BenchmarkDescriptor& benchmark)
		{
			return BenchmarkDescriptor(benchmark.name_, tag.ToTag() / benchmark.tag_);
		}

	private:
		std::string_view name_;

		Tag tag_;
	};

	export namespace literals
	{
		/**
		 * @brief Creates a BenchmarkDescriptor that produces Benchmark objects.
		 *	Works at suite level (converts to Test) and inside BenchmarkGenerator.
		 */
		[[nodiscard]] consteval auto operator""_bench(const char* const name, const std::size_t size)
			-> BenchmarkDescriptor
		{
			return BenchmarkDescriptor(std::string_view(name, size));
		}
	}
}
