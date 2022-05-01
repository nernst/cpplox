#pragma once

#include <memory>
#include "expr.hpp"

namespace lox {

	class statement;
	using statement_ptr = std::unique_ptr<statement>;

	// concrete statements
	class expression_stmt;
	class print_stmt;
	class var_stmt;

	class statement
	{
	public:
		class visitor
		{
		public:
			virtual ~visitor() { }

			virtual void visit(expression_stmt const& stmt) = 0;
			virtual void visit(print_stmt const& stmt) = 0;
			virtual void visit(var_stmt const& stmt) = 0;
		};

	public:
		statement() = default;
		statement(statement const&) = delete;
		statement(statement&&) = default;

		statement& operator=(statement const&) = delete;
		statement& operator=(statement&&) = default;

		virtual ~statement() { }

		virtual void accept(visitor& v) const = 0;

		template<typename T, typename... Args>
		static statement_ptr make(Args... args)
		{ return std::make_unique<T>(std::forward<Args>(args)...); }
	};


	class expression_stmt : public statement
	{
	public:
		explicit expression_stmt(expression_ptr&& expr)
		: expr_{std::move(expr)}
		{ }

		expression const& expr() const { return *expr_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr expr_;
	};


	class print_stmt : public statement
	{
	public:
		explicit print_stmt(expression_ptr&& expr)
		: expr_{std::move(expr)}
		{ }

		expression const& expr() const { return *expr_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr expr_;
	};

	class var_stmt : public statement
	{
	public:
		explicit var_stmt(token&& name, expression_ptr&& initializer)
		: name_{std::move(name)}
		, initializer_{std::move(initializer)}
		{ }

		token const& name() const { return name_; }
		expression_ptr const& initializer() const { return initializer_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		token name_;
		expression_ptr initializer_;
	};

} // namespace lox

