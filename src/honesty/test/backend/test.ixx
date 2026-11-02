export module synodic.honesty.test.backend:test;

import std;

import function_ref;
import inplace_vector;

import synodic.honesty.metric;

import :requirements;
import :tag;

namespace honesty::test
{
	export class Test;
	export using Generator = std::generator<Test>;

	// Forward declare Benchmark for BenchmarkGenerator
	export class Benchmark;

	/**
	 * @brief Generator that yields only Benchmark cases. Used for benchmark groups to
	 *	enforce at compile-time that only benchmarks (not tests or subtests) can be yielded.
	 */
	export using BenchmarkGenerator = std::generator<Benchmark>;

	// TODO: Make this a compile-time check
	export constexpr void VerifyTestName(const std::string_view name)
	{
		// Empty check
		if (name.empty())
		{
			throw std::runtime_error(std::format("Empty suite name"));
		}

		// Uppercase check
		std::ranges::for_each(
			name,
			[name](const char character)
			{
				// Uppercase check
				if (std::isupper(character))
				{
					throw std::runtime_error(std::format("Uppercase suite name '{}'", name));
				}

				// Space check
				if (std::isspace(character))
				{
					throw std::runtime_error(std::format("Suite name '{}' contains spaces", name));
				}

				// Check for non-alphanumeric characters, excluding '_'
				if (!std::isalnum(character) && character != '_')
				{
					throw std::runtime_error(std::format("Non-alphanumeric suite name '{}'", name));
				}
			});
	}

	/**
	 * @brief A benchmark case that can contain either a single measurement function
	 *	or a nested benchmark group. Used within BenchmarkGenerator to enforce at
	 *	compile-time that benchmark groups cannot contain regular tests or subtests.
	 */
	export class Benchmark final
	{
	public:
		using FunctionType = std::function_ref<void(metric::RegressionContext&)>;
		using GroupType = std::function_ref<BenchmarkGenerator()>;
		using VariantType = std::variant<FunctionType, GroupType>;

		/// Construct a single benchmark case
		constexpr Benchmark(
			const std::string_view name,
			const Tag& tag,
			const FunctionType& function) :
			name_(name),
			tag_(tag),
			variant_(function)
		{
			VerifyTestName(name);
		}

		/// Construct a nested benchmark group
		constexpr Benchmark(
			const std::string_view name,
			const Tag& tag,
			const GroupType& group) :
			name_(name),
			tag_(tag),
			variant_(group)
		{
			VerifyTestName(name);
		}

		std::string_view Name() const { return name_; }
		const Tag& Tags() const { return tag_; }
		const VariantType& Variant() const { return variant_; }

		bool IsGroup() const { return std::holds_alternative<GroupType>(variant_); }

	private:
		std::string_view name_;
		Tag tag_;
		VariantType variant_;
	};

	// Forward declaration
	export
	class TestData;

	class Test final
	{
		using VariantType = std::variant<
			std::monostate,
			std::function_ref<void(const Requirements&)>,
			std::function_ref<Generator(const Requirements&)>,
			std::function_ref<Generator()>,
			std::function_ref<void(metric::RegressionContext&)>,
			std::function_ref<BenchmarkGenerator()>
		>;

	public:
		constexpr Test(
			const std::string_view name,
			const Tag& tag,
			const std::function_ref<void(const Requirements&)>& test,
			[[maybe_unused]] std::string_view description = "") :
			name_(name),
			test_(test),
			tag_(tag)
		{
			VerifyTestName(name);
		}

		constexpr Test(
			const std::string_view name,
			const Tag& tag,
			const std::function_ref<Generator(const Requirements&)>& test,
			[[maybe_unused]] std::string_view description = "") :
			name_(name),
			test_(test),
			tag_(tag)
		{
			VerifyTestName(name);
		}

		constexpr Test(
			const std::string_view name,
			const Tag& tag,
			const std::function_ref<Generator()>& test,
			[[maybe_unused]] std::string_view description = "") :
			name_(name),
			test_(test),
			tag_(tag)
		{
			VerifyTestName(name);
		}

		/**
		 * @brief Constructs a benchmark test with an injected RegressionContext
		 */
		constexpr Test(
			const std::string_view name,
			const Tag& tag,
			const std::function_ref<void(metric::RegressionContext&)>& benchmark,
			[[maybe_unused]] std::string_view description = "") :
			name_(name),
			test_(benchmark),
			tag_(tag)
		{
			VerifyTestName(name);
		}

		/**
		 * @brief Constructs a benchmark group that collects child benchmarks for comparison
		 */
		constexpr Test(
			const std::string_view name,
			const Tag& tag,
			const std::function_ref<BenchmarkGenerator()>& benchmarkGroup,
			[[maybe_unused]] std::string_view description = "") :
			name_(name),
			test_(benchmarkGroup),
			tag_(tag)
		{
			VerifyTestName(name);
		}

		/**
		 * @brief Implicit conversion from Benchmark to Test for suite-level benchmark support.
		 *	Allows `_bench` to be used at suite level by yielding to Generator.
		 */
		Test(const Benchmark& benchmark) :
			name_(benchmark.Name()),
			tag_(benchmark.Tags())
		{
			if (std::holds_alternative<Benchmark::FunctionType>(benchmark.Variant()))
			{
				test_ = std::get<Benchmark::FunctionType>(benchmark.Variant());
			}
			else
			{
				test_ = std::get<Benchmark::GroupType>(benchmark.Variant());
			}
		}

		constexpr Test(const Test& other)                = delete;
		constexpr Test(Test&& other) noexcept            = delete;
		constexpr Test& operator=(const Test& other)     = delete;
		constexpr Test& operator=(Test&& other) noexcept = delete;

		std::string_view Name() const
		{
			return name_;
		}

		std::span<const Tag::value_type> Tags() const
		{
			return tag_.View();
		}

	private:
		friend TestData;

		std::string_view name_;
		VariantType test_;

		Tag tag_;
	};

	/**
	 * @brief Data that can be extracted from a test object
	 */
	class TestData
	{
	public:
		explicit constexpr TestData(const Test& test) :
			test_(test)
		{
		}

		std::string_view Name() const
		{
			return test_.name_;
		}

		Test::VariantType Variant() const
		{
			return test_.test_;
		}

		const Tag& Tag() const
		{
			return test_.tag_;
		}

	private:
		const Test& test_;
	};
}
