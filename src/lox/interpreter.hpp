#pragma once

#include "exceptions.hpp"
#include "expr.hpp"
#include "literal_holder.hpp"

namespace lox {

template<class> inline constexpr bool always_false_v = false;


class interpreter : public visitor
{
public:

	using value_t = literal_holder;

	static value_t evaluate(expression const& expr)
	{
		interpreter inter;
		inter.visit(expr);
		return inter.result();
	}

	value_t const& result() const { return result_; }

	void visit(expression const& expr) override
	{
		expr.accept(*this);
	}

	void visit(unary const& unary) override
	{
		auto right = evaluate(unary.right());

		switch(unary.op_token().type())
		{
			case token_type::BANG:
				result_ = !is_truthy(right);
				break;
				
			case token_type::MINUS:
				{
					auto visitor = [](auto&& arg) -> value_t
					{
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, double>)
							return -(arg);
						else
							LOX_THROW(type_error, fmt::format("invalid type for unary '-': {}", type(arg)));
					};
					result_ = std::visit(visitor, right);
				}
				break;
		
			default:
				LOX_THROW(programming_error, fmt::format("unsupported token: {}", unary.op_token().type()));		
		}
	}

	void visit(binary const& binary) override
	{
		value_t left{evaluate(binary.left())};
		value_t right{evaluate(binary.right())};

		auto is_equal = [&left, &right]() -> bool
		{
			auto outer = [&right](auto&& larg) -> bool
			{
				auto inner = [&larg](auto&& rarg) -> bool
				{
					using T = std::decay_t<decltype(larg)>;
					using U = std::decay_t<decltype(rarg)>;

					if constexpr (std::is_same_v<T, U>)
						return larg == rarg;
					else
						return false;
				};
				return std::visit(inner, right);
			};
			return std::visit(outer, left);
		};

		switch(binary.op_token().type())
		{
			case token_type::BANG_EQUAL:
				result_ = !is_equal();
				return;

			case token_type::EQUAL_EQUAL:
				result_ = is_equal();
				return;

			default:
				break;
		}

#define LOX_RAISE LOX_THROW(type_error, \
				fmt::format( \
					"incompatible types for binary '{}': {} and {}", \
					binary.op_token().lexeme(), \
					type(left), \
					type(right)\
				) \
			)

		if (!is_same_type(left, right))
			LOX_RAISE;

		switch(binary.op_token().type())
		{

#define NUMERIC_OP(token, op) \
				case token_type::token: \
				{ \
					auto visitor = [&](auto&& arg) -> double \
					{ \
						using T = std::decay_t<decltype(arg)>; \
						if constexpr (std::is_same_v<T, double>) \
							return arg op std::get<double>(right); \
						else \
							LOX_RAISE; \
					}; \
					result_ = std::visit(visitor, left); \
				} \
				break

			NUMERIC_OP(MINUS, -);
			NUMERIC_OP(PLUS, +);
			NUMERIC_OP(SLASH, /);
			NUMERIC_OP(STAR, *);
			NUMERIC_OP(GREATER, >);
			NUMERIC_OP(GREATER_EQUAL, >=);
			NUMERIC_OP(LESS, <);
			NUMERIC_OP(LESS_EQUAL, <=);

			default:
				LOX_THROW(programming_error, fmt::format("unhandled binary operator: '{}'", binary.op_token().lexeme()));
		}

#undef NUMERIC_OP
#undef LOX_RAISE
	}

	void visit(grouping const& grouping) override
	{
		result_ = evaluate(grouping.expr());
	}

	void visit(literal const& literal) override
	{
		result_ = literal.value();
	}

private:
	value_t result_;
};

} // namespace lox

