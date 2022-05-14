#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <variant>
#include <fmt/format.h>

#include "callable.hpp"
#include "exceptions.hpp"
#include "utility.hpp"

namespace lox
{
	using std::nullptr_t;

	class object
	{
		template<typename T>
		struct accessor
		{
			T operator()(auto&& arg) const
			{
				using ActualT = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<ActualT, T>)
					return arg;

				auto msg{
					fmt::format(
						"Cannot get {}. Actual type: {}.",
						typeid(T).name(), 
						typeid(ActualT).name()
					)
				};
				throw type_error{std::move(msg)};
			}
		};

	public:

		enum class type
		{
			NIL,
			BOOL,
			DOUBLE,
			STRING,
			CALLABLE
		};

		using holder_t = std::variant<
			nullptr_t,
			bool,
			double,
			std::string,
			callable
		>;

		object()
		: value_{nullptr}
		{ }

		template<typename T>
		explicit object(T&& value)
		: value_{std::forward<T>(value)}
		{ }

		explicit object(int value)
		: value_{static_cast<double>(value)}
		{ }

		explicit object(std::string_view value)
		: value_{std::string{value}}
		{ }

		type get_type() const { return static_cast<type>(value_.index()); }

		const char* get_type_str() const
		{
			switch(get_type())
			{
				case type::STRING: return "str";
				case type::DOUBLE: return "double";
				case type::BOOL: return "bool";
				case type::NIL: return "nil";

				default:
					throw programming_error{
						fmt::format(
							"get_type_str: unhandled type: {}",
							static_cast<int>(get_type())
						)
					};
			}
		}

		std::string str() const
		{
			auto visitor = [](auto&& arg) -> std::string
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, std::string>)
					return arg;
				else if constexpr (std::is_same_v<T, double>)
				{
					auto s{std::to_string(arg)};
					assert(!s.empty());

					// trim trailing zeros in the decimal
					// if there's nothing left, also remove the decimal
					// e.g. "5.000000" should become "5",
					// "5.010000" should become "5.01"
					auto pos{s.find('.')};
					if (pos != std::string::npos)
					{
						auto i{s.size()};
						size_t count{0};
						while (i > pos && (s[i - 1] == '0' || s[i - 1] == '.'))
						{
							--i;
							++count;
						}

						s.erase(i, count);
					}
					return s;
				}
				else if constexpr (std::is_same_v<T, bool>)
					return arg ? "true" : "false";
				else if constexpr (std::is_same_v<T, nullptr_t>)
					return "nil";
				else if constexpr (std::is_same_v<T, callable>)
					return arg.str();
				else 
					static_assert(always_false_v<T>, "str: invalid visitor");
			};
			return std::visit(visitor, value_);
		}

		template<typename T>
		T get() const { return std::visit(accessor<T>(), value_); }

		explicit operator bool() const
		{
			auto visitor = [](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, bool>)
					return arg;
				if constexpr (std::is_same_v<T, nullptr_t>)
					return false;
				return true;
			};
			return std::visit(visitor, value_);
		}

		explicit operator std::string() const { return get<std::string>(); }
		explicit operator double() const {return get<double>(); }
		explicit operator nullptr_t() const { return get<nullptr_t>(); }

		object operator-() const
		{
			if (get_type() == type::DOUBLE)
				return object{-static_cast<double>(*this)};

			auto msg {fmt::format("cannot apply unary '-' to type {}.", get_type_str())};
			throw type_error{std::move(msg)};
		}

		object operator!() const
		{ return object{!static_cast<bool>(*this)}; }

		bool operator==(object const& other) const
		{
			if (get_type() != other.get_type())
				return false;

			auto visitor = [&other](auto&& arg)
			{
				using T = std::decay_t<decltype(arg)>;

				if constexpr (std::is_same_v<T, std::string>)
					return arg == std::get<std::string>(other.value_);
				else if constexpr (std::is_same_v<T, double>)
					return arg == std::get<double>(other.value_);
				else if constexpr (std::is_same_v<T, bool>)
					return arg == std::get<bool>(other.value_);
				else if constexpr (std::is_same_v<T, nullptr_t>)
					return true;
				else if constexpr (std::is_same_v<T, callable>)
					return &arg.cimpl() == &std::get<callable>(other.value_).cimpl();
				else
					static_assert(always_false_v<T>, "operator==: invalid visitor");
			};
			return std::visit(visitor, value_);
		}

		bool operator!=(object const& other) const { return !(*this == other); }

		bool operator<(object const& other) const { return numeric_op(other, "<", std::less{}); }
		bool operator<=(object const& other) const { return numeric_op(other, "<=", std::less_equal{}); }
		bool operator>(object const& other) const { return numeric_op(other, ">", std::greater{}); }
		bool operator>=(object const& other) const { return numeric_op(other, ">=", std::greater_equal{}); }

		object operator+(object const& other) const
		{
			if (get_type() != other.get_type())
				goto on_type_error;

			switch (get_type())
			{
				case type::STRING:
				case type::DOUBLE:
					return object{std::plus<>{}(static_cast<double>(*this), static_cast<double>(other))};

				default:
					break;
			}

		on_type_error:
			throw type_error{
				fmt::format(
					"unsupported operand type(s) for '+': '{}' and '{}'", 
					get_type_str(), 
					other.get_type_str()
				)
			};
		}
		object operator-(object const& other) const { return object{numeric_op(other, "-", std::minus{})}; }
		object operator/(object const& other) const { return object{numeric_op(other, "/", std::divides{})}; }
		object operator*(object const& other) const { return object{numeric_op(other, "*", std::multiplies{})}; }

	private:
		holder_t value_;

		template<typename Op>
		auto numeric_op(object const& other, const char* op_token, Op op) const -> decltype(op(0.0, 0.0))
		{
			if (get_type() != type::DOUBLE || get_type() != other.get_type())
			{
				throw type_error{
					fmt::format(
						"unsupported operand type(s) for '{}': '{}' and '{}'", 
						op_token, 
						get_type_str(), 
						other.get_type_str()
					)
				};
			}

			return op(static_cast<double>(*this), static_cast<double>(other));
		}
	};

	inline std::ostream& operator<<(std::ostream& os, object const& obj)
	{ return os << obj.str(); }

} // namespace lox

