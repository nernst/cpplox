#pragma once

#include <memory>
#include <string_view>
#include <variant>


namespace lox {

	using literal_holder = std::variant<
		std::string_view,
		double,
		bool,
		nullptr_t
	>;

	std::string str(literal_holder const& lh)
	{
		auto visitor = [](auto&& arg) -> std::string{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::string_view>)
				return std::string{arg};
			else if constexpr (std::is_same_v<T, double>)
				return std::to_string(arg);
			else if constexpr (std::is_same_v<T, bool>)
				return arg ? "true" : "false";
			else if constexpr (std::is_same_v<T, nullptr_t>)
				return "nil";
		};

		return std::visit(visitor, lh);
	}

} // namespace lox

