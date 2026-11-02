export module synodic.honesty.test.backend:suite;

import std;

import :fixture;
import function_ref;
import :test;

namespace honesty::test
{
	/**
	 * @brief Validates a suite name to ensure it meets the criteria
	 */
	export consteval bool IsValidSuiteName(const std::string_view name)
	{
		if (name.empty())
		{
			return false;
		}

		for (const char character : name)
		{
			// Check for uppercase
			if (character >= 'A' && character <= 'Z')
			{
				return false;
			}

			// Check for spaces
			if (character == ' ' || character == '\t' || character == '\n' || character == '\r')
			{
				return false;
			}

			// Check for alphanumeric or underscore
			const bool is_lowercase = character >= 'a' && character <= 'z';
			const bool is_digit = character >= '0' && character <= '9';
			const bool is_underscore = character == '_';

			if (!is_lowercase && !is_digit && !is_underscore)
			{
				return false;
			}
		}

		return true;
	}

	export consteval void VerifySuiteName(std::string_view name)
	{
		if (name.empty())
		{
			throw "Suite name cannot be empty";
		}

		if (!IsValidSuiteName(name))
		{
			throw "Suite name must contain only lowercase letters, digits, and underscores";
		}
	}

	// Forward declaration for exporting
	export class SuiteData;

	/**
	 * @brief A suite generator callable that can be reconstructed on demand: captureless lambdas and other empty,
	 * default-constructible callables. Suites store a function pointer to a thunk rather than a reference to the
	 * callable, so construction is a valid constant expression and never refers to a temporary.
	 */
	template<typename Fn>
	concept suite_generator = std::is_empty_v<Fn> && std::default_initializable<Fn>;

	using SuiteFunction		   = Generator (*)();
	using SuiteFixtureFunction = Generator (*)(Fixture&);

	/**
	 * @brief Allows the static registration of tests in the global scope. Constructed at compile-time so that test
	 * registration is constrained. We don't want a suite wrapper to be a generic fixture that can contain expensive
	 * calculations. All tests are known up-front and parameterized tests are expanded at run-time
	 */
	export template<size_t LiteralSize>
	class Suite final
	{
		static_assert(LiteralSize > 0, "LiteralSize must be greater than 0");

		static constexpr size_t NAME_SIZE = LiteralSize - 1;

		using VariantType = std::variant<SuiteFunction, SuiteFixtureFunction>;

	public:
		template<suite_generator Fn>
			requires std::is_invocable_r_v<Generator, Fn&>
		consteval Suite(const char (&name)[LiteralSize], Fn) :
			generator_(std::in_place_type<SuiteFunction>,
					   []() -> Generator
					   {
						   Fn function;
						   return function();
					   })
		{
			std::copy(std::begin(name), std::end(name) - 1, std::begin(name_));
			VerifySuiteName(std::string_view{name_.data(), name_.size()});
		}

		template<suite_generator Fn>
			requires(!std::is_invocable_v<Fn&> && std::is_invocable_r_v<Generator, Fn&, Fixture&>)
		consteval Suite(const char (&name)[LiteralSize], Fn) :
			generator_(std::in_place_type<SuiteFixtureFunction>,
					   [](Fixture& fixture) -> Generator
					   {
						   Fn function;
						   return function(fixture);
					   })
		{
			std::copy(std::begin(name), std::end(name) - 1, std::begin(name_));
			VerifySuiteName(std::string_view{name_.data(), name_.size()});
		}

		Suite(const Suite& other)	  = delete;
		Suite(Suite&& other) noexcept = delete;

		Suite& operator=(const Suite& other)	 = delete;
		Suite& operator=(Suite&& other) noexcept = delete;

		constexpr std::string_view Name() const
		{
			return {name_.data(), name_.size()};
		}

	private:
		friend class SuiteData;

		std::array<char, NAME_SIZE> name_;
		VariantType generator_;
	};

	class SuiteData
	{
		using VariantType = std::variant<std::function_ref<Generator()>, std::function_ref<Generator(Fixture&)>>;

	public:

		template<size_t LiteralSize>
		explicit(false) constexpr SuiteData(
			const Suite<LiteralSize>& suite) :
			nameView_(suite.name_),
			generator_(suite.generator_)
		{
		}

		constexpr std::string_view Name() const
		{
			return nameView_;
		}

		VariantType Variant() const
		{
			return std::visit(
				[](const auto function) -> VariantType
				{
					return VariantType(std::function_ref(function));
				},
				generator_);
		}

	private:
		std::string_view nameView_;
		std::variant<SuiteFunction, SuiteFixtureFunction> generator_;
	};
}
