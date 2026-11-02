// Wrapper around std::format for styled terminal output

module;

#include <cstdio>

#ifdef _WIN32
#	include <windows.h>
#endif

export module synodic.honesty.log:colour;

import std;

namespace
{
	bool SUPPORTS_COLOUR = []() -> bool
	{
#ifdef _WIN32
		{
			const HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

			DWORD mode = 0;
			if (consoleHandle == INVALID_HANDLE_VALUE || !GetConsoleMode(consoleHandle, &mode))
			{
				return false;
			}
			SetConsoleMode(consoleHandle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

			return true;
		}
#else
		{
			if (const char* term = std::getenv("TERM"))
			{
				return true;
			}
			return false;
		}
#endif
	}();
}

namespace honesty::log
{

	export bool SupportsColour()
	{
		return SUPPORTS_COLOUR;
	}

	/**
	 * @brief ANSI color escape codes
	 */
	export enum class TerminalColor : std::uint8_t
	{
		BLACK = 30,
		RED,
		GREEN,
		YELLOW,
		BLUE,
		MAGENTA,
		CYAN,
		WHITE,
		BRIGHT_BLACK = 90,
		BRIGHT_RED,
		BRIGHT_GREEN,
		BRIGHT_YELLOW,
		BRIGHT_BLUE,
		BRIGHT_MAGENTA,
		BRIGHT_CYAN,
		BRIGHT_WHITE
	};

	export struct Colour8
	{
		explicit constexpr Colour8(const std::uint8_t code) :
			code(code)
		{
		}

		explicit constexpr Colour8(const TerminalColor code) :
			code(std::to_underlying(code))
		{
		}

		std::uint8_t code;

		friend auto operator<=>(const Colour8&, const Colour8&) = default;
	};

	export struct Colour24
	{
		explicit constexpr Colour24(const std::uint32_t hexCode) :
			red((hexCode >> 16) & 0xFF),
			green((hexCode >> 8) & 0xFF),
			blue(hexCode & 0xFF)
		{
		}

		constexpr Colour24(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) :
			red(red),
			green(green),
			blue(blue)
		{
		}

		std::uint8_t red;
		std::uint8_t green;
		std::uint8_t blue;

		friend auto operator<=>(const Colour24&, const Colour24&) = default;
	};

	/**
	 * @brief Supported SGR parameter mask
	 */
	export enum class Attribute : std::uint8_t
	{
		NONE	  = 0,
		BOLD	  = 1,
		FAINT	  = 1 << 1,
		ITALIC	  = 1 << 2,
		UNDERLINE = 1 << 3,
		BLINK	  = 1 << 4,
		REVERSE	  = 1 << 5,
		CONCEAL	  = 1 << 6,
		STRIKE	  = 1 << 7,
	};

	constexpr size_t ATTRIBUTE_COUNT = std::numeric_limits<std::underlying_type_t<Attribute>>::digits;

	constexpr std::array<std::uint8_t, ATTRIBUTE_COUNT> ATTRIBUTE_ANSI_CODES = {
		1,	// bold
		2,	// faint
		3,	// italic
		4,	// underline
		5,	// blink
		7,	// reverse
		8,	// conceal
		9,	// strike
	};

	export using ColorType = std::variant<Colour8, Colour24>;

	// TODO: Support multiple attributes

	export class TextStyle
	{
	public:
		explicit constexpr TextStyle(const TerminalColor color, const Attribute attribute = Attribute::NONE) :
			foreground_(Colour8(color)),
			attributeMask_(std::to_underlying(attribute))
		{
		}

		explicit constexpr TextStyle(const Colour8 color, const Attribute attribute = Attribute::NONE) :
			foreground_(color),
			attributeMask_(std::to_underlying(attribute))
		{
		}

		explicit constexpr TextStyle(const Colour24 color, const Attribute attribute = Attribute::NONE) :
			foreground_(color),
			attributeMask_(std::to_underlying(attribute))
		{
		}

		constexpr std::optional<ColorType> Foreground() const
		{
			return foreground_;
		}

		constexpr std::optional<ColorType> Background() const
		{
			return background_;
		}

		constexpr std::uint8_t AttributeMask() const
		{
			return attributeMask_;
		}

		friend constexpr auto operator<=>(const TextStyle&, const TextStyle&) = default;

	private:
		std::optional<ColorType> foreground_;
		std::optional<ColorType> background_;
		std::uint8_t attributeMask_;
	};

	template<typename... Ts>
	struct Overloaded : Ts...
	{
		using Ts::operator()...;
	};

	std::uint8_t ConvertIndexToAnsiCode(const std::uint8_t maskIndex)
	{
		return ATTRIBUTE_ANSI_CODES[maskIndex];
	}

	/**
	 * @brief Application of a text style to a string
	 * @param out
	 * @param style
	 * @param fmt
	 * @param args
	 */
	template<typename OutputIt, typename BackendBuffer, typename CharT>
		requires std::output_iterator<OutputIt, const CharT&>
	OutputIt vformat_to_impl(
		OutputIt out,
		const TextStyle& style,
		std::basic_string_view<CharT> fmt,
		std::basic_format_args<std::basic_format_context<BackendBuffer, std::type_identity_t<CharT>>> args)
	{
		OutputIt next = out;

		if (const std::uint8_t attributeMask = style.AttributeMask())
		{
			constexpr size_t attributeCount = std::numeric_limits<std::underlying_type_t<Attribute>>::digits;

			for (int index = 0; index < attributeCount; ++index)
			{
				if (const std::uint8_t mask = static_cast<std::uint8_t>(1 << index); attributeMask & mask)
				{
					next = std::format_to(next, "\x1b[{}m", ConvertIndexToAnsiCode(static_cast<std::uint8_t>(index)));
				}
			}
		}

		if (std::optional<ColorType> foreground = style.Foreground())
		{
			ColorType& colorType = foreground.value();

			std::visit(
				Overloaded {
					[&](const Colour8 arg)
					{
						next = std::format_to(next, "\x1b[{}m", arg.code);
					},
					[&](const Colour24 arg)
					{
						next = std::format_to(next, "\x1b[38;2;{:03};{:03};{:03}m", arg.red, arg.green, arg.blue);
					}},
				colorType);
		}

		if (std::optional<ColorType> background = style.Background())
		{
			ColorType& colorType = background.value();

			std::visit(
				Overloaded {
					[&](const Colour8 arg)
					{
						next = std::format_to(next, "\x1b[{}m", arg.code + 10);
					},
					[&](const Colour24 arg)
					{
						next = std::format_to(next, "\x1b[48;2;{:03};{:03};{:03}m", arg.red, arg.green, arg.blue);
					}},
				colorType);
		}

		next = std::vformat_to(next, fmt, args);
		return std::format_to(next, "\x1b[0m");
	}

	/**
	 * @brief Overload (1) - https://en.cppreference.com/w/cpp/utility/format/vformat
	 */
	export std::string vformat(const TextStyle& style, const std::string_view fmt, const std::format_args args)
	{
		std::string data;
		vformat_to_impl(std::back_inserter(data), style, fmt, args);
		return data;
	}

	/**
	 * @brief Overload (1) - https://en.cppreference.com/w/cpp/utility/format/vformat_to
	 */
	export template<std::output_iterator<const char&> OutputIt>
	OutputIt vformat_to(OutputIt out, const TextStyle& style, const std::string_view fmt, const std::format_args args)
	{
		if (SupportsColour())
		{
			return vformat_to_impl(out, style, fmt, args);
		}

		return std::vformat_to(out, fmt, args);
	}

	/**
	 * @brief  Overload (1) - https://en.cppreference.com/w/cpp/utility/format/format
	 */
	export template<typename... Args>
	std::string format(const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			return vformat(style, fmt.get(), std::make_format_args(args...));
		}

		return std::format(fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Overload (1) - https://en.cppreference.com/w/cpp/utility/format/format_to
	 */
	export template<std::output_iterator<const char&> OutputIt, typename... Args>
	OutputIt format_to(OutputIt out, const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			return vformat_to(std::move(out), style, fmt.get(), std::make_format_args(args...));
		}

		return std::format_to(std::move(out), fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Overload (1) - https://en.cppreference.com/w/cpp/io/print
	 */
	export template<typename... Args>
	void print(std::FILE* stream, const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			std::string data;
			vformat_to_impl(std::back_inserter(data), style, fmt, args);
			std::print(stream, "{}", data);
		}
		else
		{
			std::print(stream, fmt, std::forward<Args>(args)...);
		}
	}

	/**
	 * @brief Overload (2) - https://en.cppreference.com/w/cpp/io/print
	 */
	export template<typename... Args>
	void print(const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			return print(stdout, style, fmt, std::forward<Args>(args)...);
		}

		return std::print(fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Overload (1) - https://en.cppreference.com/w/cpp/io/println
	 */
	export template<typename... Args>
	void println(std::FILE* stream, const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			print(stream, style, "{}\n", std::format(fmt, args...));
		}
		else
		{
			std::println(stream, fmt, std::forward<Args>(args)...);
		}
	}

	/**
	 * @brief Overload (2) - https://en.cppreference.com/w/cpp/io/println
	 */
	export template<typename... Args>
	void println(const TextStyle& style, std::format_string<Args...> fmt, Args&&... args)
	{
		if (SupportsColour())
		{
			println(stdout, style, fmt, args...);
		}
		else
		{
			std::println(fmt, std::forward<Args>(args)...);
		}
	}
}

export template<>
struct std::formatter<honesty::log::Colour24> :
	std::formatter<std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>>
{
	template<typename Context>
	auto format(honesty::log::Colour24 colour, Context& context) const
	{
		return std::formatter<std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>>::format(
			std::make_tuple(colour.red, colour.green, colour.blue),
			context);
	}
};
