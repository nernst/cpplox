#pragma once

#include <cassert>
#include <ranges>
#include <vector>
#include <unordered_map>
#include "exceptions.hpp"
#include "interpreter.hpp"

namespace lox
{
	class resolver : expression::visitor, statement::visitor
	{
		using scope_t = std::unordered_map<std::string, bool>;
		using stack_t = std::vector<scope_t>;

		enum class function_type
		{
			NONE,
			FUNCTION
		};

	public:
		explicit resolver(std::ostream& error, interpreter& inter)
		: error_{&error}
		, inter_{&inter}
		{ }

		resolver() = delete;
		resolver(resolver const&) = delete;
		resolver(resolver&&) = default;

		resolver& operator=(resolver const&) = delete;
		resolver& operator=(resolver&&) = default;

		bool had_error() const { return had_error_; }

		void resolve(statement const& statement)
		{
			statement.accept(*this);
		}

		void resolve(std::vector<statement_ptr> const& statements)
		{
			std::ranges::for_each(statements, [this](auto&& p){ resolve(*p); });
		}

		//
		// statements
		//
		void visit(block_stmt const& stmt) override
		{
			begin_scope();
			resolve(stmt.statements());
			end_scope();
		}

		void visit(expression_stmt const& stmt) override
		{
			resolve(stmt.expr());
		}

		void visit(func_stmt const& stmt) override
		{
			declare(stmt.name());
			define(stmt.name());
			resolve_function(stmt, function_type::FUNCTION);
		}

		void visit(if_stmt const& stmt) override
		{
			resolve(stmt.condition());
			resolve(stmt.then_branch());
			if (stmt.else_branch())
				resolve(*stmt.else_branch());
		}

		void visit(print_stmt const& stmt) override
		{
			resolve(stmt.expr());
		}

		void visit(return_stmt const& stmt) override
		{
			if (current_function_ == function_type::NONE)
				error(stmt.keyword(), "Cannot return from top-level code.");

			if (stmt.value())
				resolve(*stmt.value());
		}

		void visit(var_stmt const& stmt) override
		{
			declare(stmt.name());
			if (stmt.initializer())
				resolve(*stmt.initializer());
			define(stmt.name());
		}

		void visit(while_stmt const& stmt) override
		{
			resolve(stmt.condition());
			resolve(stmt.body());
		}


		//
		// expressions
		// 
		void visit(assign const& expr) override
		{
			resolve(expr.value());
			resolve_local(expr, expr.name_token());
		}

		void visit(binary const& expr) override
		{
			resolve(expr.left());
			resolve(expr.right());
		}

		void visit(call const& expr) override
		{
			resolve(expr.callee());
			for (auto&& arg : expr.arguments())
				resolve(*arg);
		}

		void visit(grouping const& expr) override
		{
			resolve(expr.expr());
		}

		void visit(literal const& expr) override
		{
			ignore_unused(expr);
			// noop
		}

		void visit(logical const& expr) override
		{
			resolve(expr.left());
			resolve(expr.right());
		}

		void visit(unary const& expr) override
		{
			resolve(expr.right());
		}

		void visit(variable const& expr) override
		{
			if (!scopes_.empty())
			{
				auto state{get(scopes_.back(), expr.name())};
				if (state && *state == false)
					error(expr.name_token(), "Cannot read local variable in its own initializer.");
			}

			resolve_local(expr, expr.name_token());
		}

	private:	
		std::ostream* error_;
		interpreter* inter_;
		stack_t scopes_;
		function_type current_function_ = function_type::NONE;
		bool had_error_ = false;
	
		template<typename K, typename V>
		inline std::optional<V> get(std::unordered_map<K, V> const& map, K&& key)
		{
			auto i{map.find(std::forward<K>(key))};
			if (i == std::cend(map))
				return std::nullopt;
			return i->second;
		}

		void resolve(expression const& expr)
		{
			expr.accept(*this);
		}

		void begin_scope()
		{
			scopes_.emplace_back();
		}

		void end_scope()
		{
			assert(!scopes_.empty());
			scopes_.pop_back();
		}

		void declare(token const& name)
		{
			if (scopes_.empty())
				return;

			auto& scope = scopes_.back();
			auto [_, inserted] = scope.insert(std::make_pair(std::string{name.lexeme()}, false));
			if (!inserted)
				error(name, "Already a variable with this name in this scope.");
		}

		void define(token const& name)
		{
			if (scopes_.empty())
				return;

			auto& scope = scopes_.back();
			scope.insert_or_assign(std::string{name.lexeme()}, true);
		}

		void resolve_local(expression const& expr, token const& name)
		{
			for (size_t i = scopes_.size(); i > 0; --i)
			{
				auto&& scope = scopes_[i - 1];
				if (scope.find(std::string{name.lexeme()}) != scope.end())
				{
					inter_->resolve(expr, scopes_.size() - i);
					return;
				}
			}
		}

		void resolve_function(func_stmt const& stmt, function_type type)
		{
			auto enclosing_function{current_function_};
			current_function_ = type;

			begin_scope();
			for (auto&& param : stmt.parameters())
			{
				declare(param);
				define(param);
			}
			resolve(stmt.body());
			end_scope();
			current_function_ = enclosing_function;
		}

		void error(token const& where, std::string_view message)
		{
#ifdef LOX_ENV_TRACE
			for (auto&& scope : scopes_ | std::views::reverse)
			{
				*error_ << "scope @" << &scope << std::endl;
				if (scope.empty())
					*error_ << "<empty>" << std::endl;
				else
				{
					for (auto&& p : scope)
					{
						*error_ << '\t' << p.first << ": " << std::boolalpha << p.second << std::endl;
					}
				}
			}
#endif
			had_error_ = true;
			::lox::log_error(*error_, where, message);
		}
	};

} // namespace lox

