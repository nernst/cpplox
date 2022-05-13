#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include "expr.hpp"

namespace lox {

	class statement;
	using statement_ptr = std::unique_ptr<statement>;

	// concrete statements
	class block_stmt;
	class expression_stmt;
	class func_stmt;
	class if_stmt;
	class print_stmt;
	class var_stmt;
	class while_stmt;

	class statement
	{
	public:
		class visitor
		{
		public:
			virtual ~visitor() { }

			virtual void visit(block_stmt const& stmt) = 0;
			virtual void visit(expression_stmt const& stmt) = 0;
			virtual void visit(func_stmt const& stmt) = 0;
			virtual void visit(if_stmt const& smt) = 0;
			virtual void visit(print_stmt const& stmt) = 0;
			virtual void visit(var_stmt const& stmt) = 0;
			virtual void visit(while_stmt const& stmt) = 0;
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

	class block_stmt : public statement
	{
	public:
		using statements_t = std::vector<statement_ptr>;

		explicit block_stmt(statements_t&& statements)
		: statements_{std::move(statements)}
		{ }

		statements_t const& statements() const { return statements_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		statements_t statements_;
	};

	class func_stmt : public statement
	{
	public:
		using parameters_t = std::vector<token>;
		using statements_t = std::vector<statement_ptr>;

		struct impl
		{
			token name_;
			parameters_t params_;
			statements_t body_;
		};

		explicit func_stmt(token&& name, parameters_t&& params, statements_t&& body)
		:	impl_{std::make_shared<impl>(std::move(name), std::move(params), std::move(body))}
		{ }

		func_stmt(func_stmt const& copy)
		: impl_{copy.impl_}
		{ }

		func_stmt(func_stmt&& from)
		: impl_{std::move(from.impl_)}
		{ }

		func_stmt& operator=(func_stmt const& copy)
		{
			if (this != &copy)
				impl_ = copy.impl_;
			return *this;
		}

		func_stmt& operator=(func_stmt&& from)
		{
			assert(this != &from);
			impl_ = std::move(from.impl_);
			return *this;
		}

		token const& name() const { return impl_->name_; }
		parameters_t const& parameters() const { return impl_->params_; }
		statements_t const& body() const { return impl_->body_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		std::shared_ptr<impl> impl_;
	};

	class if_stmt : public statement
	{
	public:
		explicit if_stmt(expression_ptr&& condition, statement_ptr&& then_branch, statement_ptr&& else_branch = statement_ptr{})
		: condition_{std::move(condition)}
		, then_branch_{std::move(then_branch)}
		, else_branch_{std::move(else_branch)}
		{
			assert(condition_);
			assert(then_branch_);
		}

		expression const& condition() const { return *condition_; }
		statement const& then_branch() const { return *then_branch_; }
		
		std::optional<std::reference_wrapper<const statement>> else_branch() const
		{ return else_branch_ ? std::make_optional(std::cref(*else_branch_)) : std::nullopt; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr condition_;
		statement_ptr then_branch_;
		statement_ptr else_branch_;
	};

	
	class while_stmt : public statement
	{
	public:
		explicit while_stmt(expression_ptr&& condition, statement_ptr&& body)
		: condition_{std::move(condition)}
		, body_{std::move(body)}
		{
			assert(condition_);
			assert(body_);
		}

		expression const& condition() const { return *condition_; }
		statement const& body() const { return *body_; }

		void accept(visitor& v) const override { v.visit(*this); }

	private:
		expression_ptr condition_;
		statement_ptr body_;
	};

} // namespace lox

