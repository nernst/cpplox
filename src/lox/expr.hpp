#pragma once
#include <cassert>
#include <memory>
#include <sstream>
#include <typeinfo>

#include "token.hpp"
#include "utility.hpp"


namespace lox {

	class expression;
	class unary;
	class binary;
	class grouping;
	class literal;
	class variable;
	
	using expression_ptr = std::unique_ptr<expression>;

	class expression
	{
	public:
		class visitor
		{
		public:
			virtual ~visitor() {}

			virtual void visit(unary const& expr) = 0;
			virtual void visit(binary const& expr) = 0;
			virtual void visit(grouping const& expr) = 0;
			virtual void visit(literal const& expr) = 0;
			virtual void visit(variable const& expr) = 0;
		};

	public:
		expression() = default;
		expression(expression const&) = default;
		expression(expression&&) = default;

		expression& operator=(expression const&) = default;
		expression& operator=(expression&&) = default;

		virtual ~expression() { }

		virtual void accept(visitor& v) const = 0;

		template<class ExprType, class... Args>
		static expression_ptr make(Args&&... args)
		{ return std::make_unique<ExprType>(std::forward<Args>(args)...); }
	};

	class unary : public expression
	{
	public:

		unary(token&& op_token, expression_ptr&& right)
		: op_token_{std::move(op_token)}
		, right_{std::move(right)}
		{
			assert(right_.get());
		}

		unary() = delete;
		unary(unary const&) = delete;
		unary(unary&& other) = default;
		unary& operator=(unary const&) = delete;
		unary& operator=(unary&& other) = default;

		token op_token() const { return op_token_; }
		expression const& right() const { return *right_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		token op_token_;
		expression_ptr right_;
	};

	class binary : public expression
	{
	public:

		binary(expression_ptr&& left, token&& op_token, expression_ptr&& right)
		: left_{std::move(left)}
		, op_token_{std::move(op_token)}
		, right_{std::move(right)}
		{
			assert(left_.get());
			assert(right_.get());
		}

		binary(binary const&) = delete;
		binary(binary&&) = default;

		binary& operator=(binary const&) = delete;
		binary& operator=(binary&&) = default;

		expression const& left() const { return *left_; }
		token op_token() const { return op_token_; }
		expression const& right() const { return *right_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr left_;
		token op_token_;
		expression_ptr right_;
	};

	class grouping : public expression
	{
	public:

		explicit grouping(expression_ptr&& expr)
		: expr_{std::move(expr)}
		{
			assert(expr_.get());
		}

		grouping(grouping const&) = delete;
		grouping(grouping&&) = default;

		grouping& operator=(grouping const&) = delete;
		grouping& operator=(grouping&&) = default;

		expression const& expr() const { return *expr_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr expr_;
	};

	class literal : public expression
	{
	public:

		explicit literal(object&& value)
		: value_{std::move(value)}
		{ }

		explicit literal(object const& value)
		: value_{value}
		{ }

		explicit literal(double value)
		: value_{value}
		{ }

		explicit literal(const char* value)
		: value_{std::string{value}}
		{ }

		explicit literal(bool value)
		: value_{value}
		{ }

		explicit literal(nullptr_t value)
		: value_{value}
		{ }


		literal(literal const&) = delete;
		literal(literal&&) = default;

		literal& operator=(literal const&) = delete;
		literal& operator=(literal&&) = default;

		template<typename T>
		explicit operator T() const
		{ return std::get<T>(value_); }

		object const& value() const { return value_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		object value_;
	};

	
	class variable : public expression
	{
	public:
		template<typename T>
		explicit variable(T&& name)
		: name_{std::forward<T>(name)}
		{ }

		variable(variable const&) = delete;
		variable(variable&&) = default;

		variable& operator=(variable const&) = delete;
		variable& operator=(variable&&) = default;

		std::string name() const { return name_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		std::string name_;
	};

} // namespace lox

