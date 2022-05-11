#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <fmt/format.h>

#include "object.hpp"

namespace lox {

	class environment;
	using environment_ptr = std::shared_ptr<environment>;

	class environment
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
#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::assign(name: '" << name << "', value: [" << value.str() << "])" << std::endl;
#endif
			auto i = values_.find(name);
			if (i == end(values_))
			{
				if (enclosing_ != nullptr)
				{
					enclosing_->assign(name, std::forward<U>(value));
					return;
				}

				undefined(name);
			}
			else
				i->second = std::forward<U>(value);
		}

		object get(std::string const& name) const
		{
			// std::cerr << "environment::get this=" << this << ", name=" << name << std::endl;
			auto i = values_.find(name);
			if (i == values_.end())
			{
				if (enclosing_ != nullptr)
				{
					auto obj{enclosing_->get(name)};
#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::get(name: '" << name << "') ret=[" << obj.str() << "]" << std::endl;
#endif
					return obj;
				}

#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::get(name: '" << name << "') *undefined*" << std::endl;
#endif
				undefined(name);
			}

#ifdef LOX_ENV_TRACE
			std::cerr << "env[" << this << "]::assign(name: '" << name << "') ret=[" << i->second.str() << "]" << std::endl;
#endif
			return i->second;
		}

	private:
		environment_ptr enclosing_;
		value_map values_;
	};

	class scope_stack;

	class scope
	{
	public:
		explicit scope(scope_stack* scope);
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

		[[nodiscard]] environment_ptr push()
		{
			if (stack_.empty())
				stack_.emplace_back(std::make_shared<environment>());
			else
				stack_.emplace_back(std::make_shared<environment>(stack_.back()));
			return stack_.back();
		}

		void pop()
		{
			assert(stack_.size() > 1); // don't want to pop the global env
			stack_.pop_back();
		}
	};


	inline scope::scope(scope_stack* stack)
	: stack_{stack}
	{
		assert(stack_ != nullptr);
		env_ = stack_->push();
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
