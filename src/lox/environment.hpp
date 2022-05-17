#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <fmt/format.h>

#include "object.hpp"

namespace lox {

	class environment;
	using environment_ptr = std::shared_ptr<environment>;

	class environment : public std::enable_shared_from_this<environment>
	{
		using value_map = std::unordered_map<std::string, object>;

		[[noreturn]]
		static void undefined(std::string const& name)
		{
			throw runtime_error{fmt::format("Undefined variable '{}'.", name)};
		}

	public:
		explicit environment(environment_ptr enclosing = environment_ptr{})
		: enclosing_{enclosing}
		{ }

		environment(environment const&) = delete;
		environment(environment&&) = default;

		environment& operator=(environment const&) = delete;
		environment& operator=(environment&&) = default;

		environment_ptr const& enclosing() const { return enclosing_; }

		environment_ptr ancestor(std::size_t distance)
		{
			environment_ptr env{shared_from_this()};

			for (std::size_t i{0}; i < distance; ++i)
			{
				assert(env->enclosing());
				env = env->enclosing();
			}

			return env;
		}

		object get_at(std::size_t distance, std::string const& name)
		{
			auto env{ancestor(distance)};
			auto i{env->values_.find(name)};
			if (i == env->values_.end())
				return {};
			return i->second;
		}
		
		template<typename Token, typename T>
		void assign_at(std::size_t distance, Token const& name, T&& value)
		{
			assign_at(distance, std::string{name.lexeme()}, std::forward<T>(value));
		}

		template<typename T>
		void assign_at(std::size_t distance, std::string const& name, T&& value)
		{
			ancestor(distance)->values_.insert_or_assign(name, std::forward<T>(value));
		}

		template<typename T, typename U>
		void define(T&& name, U&& value)
		{
#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::define(name: '" << name << "', value: [" << value.str() << "])" << std::endl;
#endif
			values_.insert_or_assign(
				std::forward<T>(name),
				std::forward<U>(value)
			);
		}

		template<typename U>
		void assign(std::string const& name, U&& value)
		{
			auto env{this};
			while (env != nullptr)
			{
#ifdef LOX_ENV_TRACE
				std::cerr << "env[" << env << "]::assign(name: '" << name << "', value: [" << value.str() << "])" << std::endl;
#endif
				auto i = env->values_.find(name);
				if (i == end(env->values_))
				{
					env = env->enclosing_.get();
				}
				else
				{
					i->second = std::forward<U>(value);
					return;
				}
			}

			undefined(name);
		}

		object get(std::string const& name) const
		{
			auto env{this};

			while (env != nullptr)
			{
#ifdef LOX_ENV_TRACE
				std::cerr << "env[" << env << ", enclosing: " << env->enclosing_.get() << "]::get(name: '" << name << "') ..." << std::endl;
#endif
				auto i = env->values_.find(name);
				if (i == env->values_.end())
				{
					env = env->enclosing_.get();
				}
				else
				{
#ifdef LOX_ENV_TRACE
					std::cerr << "env[" << env << ", enclosing: " << env->enclosing_.get() << "]::get(name: '" << name << "') ret = " << i->second.str() << std::endl;
#endif

					return i->second;
				}
			}
			
#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::get(name: '" << name << "') *undefined*" << std::endl;
#endif
			undefined(name);
		}

		std::vector<std::string> names() const
		{
			std::vector<std::string> names;
			std::ranges::copy(
				std::views::transform(values_, [](auto&& p) { return p.first; }),
				std::back_inserter(names)
			);
			return names;
		}

	private:
		environment_ptr enclosing_;
		value_map values_;
	};

	class scope_stack;

	class scope
	{
	public:
		explicit scope(scope_stack* scope, environment_ptr closure = {});
		~scope();

		scope(scope const&) = delete;
		scope(scope&&) = delete;

		scope& operator=(scope const&) = delete;
		scope& operator=(scope&&) = delete;

		[[nodiscard]] environment& env()
		{
			assert(env_);
			return *env_;
		}

	private:
		scope_stack* stack_;
		environment_ptr env_;
	};

	class scope_stack
	{
		friend class scope;

	public:
		// TODO: max depth?
		// TODO: clone (for threading)?
		// TODO: pooled allocator?

		scope_stack()
		{
			// create global scope
			(void)push();
		}

		scope_stack(scope_stack const&) = delete;
		scope_stack(scope_stack&&) = default;

		scope_stack& operator=(scope_stack const&) = delete;
		scope_stack& operator=(scope_stack&&) = default;

		[[nodiscard]] environment& current() { assert(!stack_.empty()); return *stack_.back(); }
		[[nodiscard]] environment& global() { assert(!stack_.empty()); return *stack_.front(); }

	private:
		std::vector<environment_ptr> stack_;

		[[nodiscard]] environment_ptr push(environment_ptr closure = {})
		{
			if (stack_.empty())
				stack_.emplace_back(std::make_shared<environment>());
			else
				stack_.emplace_back(std::make_shared<environment>(closure ? closure : stack_.back()));
			return stack_.back();
		}

		void pop()
		{
			assert(stack_.size() > 1); // don't want to pop the global env
			stack_.pop_back();
		}
	};


	inline scope::scope(scope_stack* stack, environment_ptr closure)
	: stack_{stack}
	{
		assert(stack_ != nullptr);
		env_ = stack_->push(closure);
		assert(env_ != nullptr);
	}

	inline scope::~scope()
	{
		assert(stack_ != nullptr);
		assert(env_ != nullptr);
		assert(&stack_->current() == env_.get());

		stack_->pop();
		stack_ = nullptr;
		env_.reset();
	}


} // namespace lox
