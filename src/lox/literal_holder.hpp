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

	inline std::string str(literal_holder const& lh)
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

	inline bool is_truthy(literal_holder const& lh)
	{
		auto visitor = [](auto&& arg) -> bool
		{
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, bool>)
				return arg;
			else if constexpr (std::is_same_v<T, nullptr_t>)
				return false;
			else
				return true;
		};
		return std::visit(visitor, lh);
	}

	inline bool is_same_type(literal_holder const& lhs, literal_holder const& rhs)
	{
		auto outer = [&rhs](auto&& larg) -> bool
		{
			auto inner = [&larg](auto&& rarg) -> bool
			{
				using T = std::decay_t<decltype(larg)>;
				using U = std::decay_t<decltype(rarg)>;

				return std::is_same_v<T, U>;
			};

			return std::visit(inner, rhs);
		};

		return std::visit(outer, lhs);
	}


	inline constexpr const char* type(double) { return "double"; }
	inline constexpr const char* type(bool) { return "bool"; }
	inline constexpr const char* type(std::string_view const&) { return "str"; }
	inline constexpr const char* type(nullptr_t) { return "nil"; }

	inline const char* type(literal_holder const& lh)
	{
		auto visitor = [](auto&& arg) -> const char*
		{
			return type(arg);
		};

		return std::visit(visitor, lh);
	}

} // namespace lox

