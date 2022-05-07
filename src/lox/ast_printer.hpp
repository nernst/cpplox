#pragma once

#include <sstream>

#include "expr.hpp"

namespace lox 
{

	class ast_printer : public expression::visitor
	{
	public:
		~ast_printer() = default;
		ast_printer() = default;
		ast_printer(ast_printer const&) = default;
		ast_printer(ast_printer &&) = default;

		ast_printer& operator=(ast_printer const&) = default;
		ast_printer& operator=(ast_printer&&) = default;

		void visit(unary const& expr) override;
		void visit(binary const& expr) override;
		void visit(grouping const& expr) override;
		void visit(literal const& expr) override;
		void visit(variable const& expr) override;
		void visit(assign const& expr) override;

		std::string const& result() const { return result_; }

		static std::string print(expression const& expr);

	private:
		std::string result_;

		template<class... Args>
		static std::string parenthesize(Args&&... args)
		{
			std::ostringstream os;
			os << "(";
			bool need_space{false};

			auto print = [&](auto&& arg)
				{
					if (need_space)
						os << " ";
					else
						need_space = true;

					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
						os << arg;
					else if constexpr (std::is_same_v<T, object>)
						os << arg.str();
					else
						os << "<~~ missing type overload: " << typeid(arg).name() << " ~~>";
				};


			int dummy[sizeof...(args)] = {(print(args), 0)...};
			ignore_unused(dummy);

			os << ")";

			return os.str();
		}
	};

	inline std::string ast_printer::print(expression const& expr)
	{
		ast_printer printer;
		expr.accept(printer);
		return printer.result();
	}

	inline void ast_printer::visit(unary const& expr)
	{
		result_ = parenthesize(expr.op_token().lexeme(), print(expr.right()));
	}

	inline void ast_printer::visit(binary const& expr)
	{
		result_ = parenthesize(expr.op_token().lexeme(), print(expr.left()), print(expr.right()));
	}

	inline void ast_printer::visit(grouping const& expr)
	{
		result_ = parenthesize("grouping", print(expr.expr()));
	}

	inline void ast_printer::visit(literal const& expr)
	{
		result_ = expr.value().str();
	}

	inline void ast_printer::visit(variable const& expr)
	{
		result_ = expr.name();
	}

	inline void ast_printer::visit(assign const& expr)
	{
		result_ = parenthesize("=", expr.name(), print(expr.value()));
	}


} // namespace lox

