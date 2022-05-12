#pragma once

#include <cassert>
#include <memory>
#include <vector>
#include <fmt/format.h>

namespace lox
{
	class interpreter;
	class object;

	class callable
	{
	public:

		struct impl
		{
			virtual ~impl() = default;
			virtual size_t arity() const = 0;
			virtual object call(interpreter& inter, std::vector<object> const& arguments) = 0;
			virtual std::string name() const = 0;
			virtual std::string str() const = 0;
		};

		struct builtin : impl
		{
			std::string str() const override final { return fmt::format("<builtin fn {} at {}>", name(), reinterpret_cast<const void*>(this)); }
		};

		explicit callable(std::shared_ptr<impl>&& cimpl)
		: impl_{std::move(cimpl)}
		{
			assert(impl_);
		}

		callable() = default;
		callable(callable const&) = default;
		callable(callable&& other)
		: impl_{other.impl_}
		{ }

		callable& operator=(callable const&) = default;
		callable& operator=(callable&& other)
		{
			// we don't really move, just share the impl.
			if (this != &other)
			{
				impl_ = other.impl_;
			}
			return *this;
		}

		bool valid() const { return static_cast<bool>(impl_); }

		std::string name() const { return impl_->name(); }
		size_t arity() const { return impl_->arity(); }
		object operator()(interpreter& inter, std::vector<object> const& arguments) const;
		std::string str() const { return impl_->str(); }

		impl& cimpl() const { return *impl_; }


		template<class Impl>
		static callable make()
		{
			return callable{std::make_shared<Impl>()};
		}

		static std::vector<callable> builtins();

	private:
		std::shared_ptr<impl> impl_;
	};

} // namespace lox

