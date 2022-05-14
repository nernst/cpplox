#include <iterator>
#include <ranges>
#include <sstream>
#include "lox/callable.hpp"
#include "lox/object.hpp"
#include "lox/builtins/clock.hpp"
#include "lox/builtins/dir.hpp"
#include "lox/interpreter.hpp"
#include "lox/statement.hpp"
#include "lox/environment.hpp"

namespace lox
{
	object callable::operator()(interpreter& inter, std::vector<object> const& arguments) const
	{ return impl_->call(inter, arguments); }

	std::vector<callable> callable::builtins()
	{
		return std::vector<callable>{{
			{ callable::make<::lox::builtin::clock>() },
			{ callable::make<::lox::builtin::dir>() },
		}};
	}

	namespace builtin
	{
		object clock::call(interpreter& inter, std::vector<object> const& args) const
		{
			ignore_unused(inter, args);

			auto now{std::chrono::high_resolution_clock::now()};
			auto tp{now.time_since_epoch()};
			
			using seconds = std::chrono::duration<double>;
			return object{std::chrono::duration_cast<seconds>(tp).count()};
		}

		object dir::call(interpreter& inter, std::vector<object> const& args) const
		{
			ignore_unused(args);

			std::vector<std::string> names;

			std::function<void(environment const&)> collector = [&](environment const& e)
			{
				auto local{e.names()};
				std::ranges::sort(local);

				if (names.empty())
				{
					names = local;
				}
				else
				{
					std::vector<std::string> work;
					std::ranges::merge(names, local, std::back_inserter(work));
					names = std::move(work);
				}

				if (e.enclosing())
					collector(*e.enclosing());
			};

			collector(inter.current_env());

			std::stringstream out;
			out << "{";
			bool need_sep{false};
			for (auto&& s : names)
			{
				if (need_sep)
					out << ", ";
				else
					need_sep = true;
				out << s;
			}
			out << "}";
			return object{out.str()};
		}
	}

	//
	// lox_function_impl
	//

	class lox_function_impl : public callable::impl
	{
	public:
		explicit lox_function_impl(func_stmt const& declaration)
		: declaration_{declaration}
		{ }

		func_stmt const& declaration() const { return declaration_; }

		size_t arity() const override
		{
			return declaration().parameters().size();
		}

		object call(interpreter& inter, std::vector<object> const& arguments) const override
		{
			// Should have already been validated, but sanity check
			assert(arguments.size() == arity());

			// push new environment
			scope s{&inter.stack()};
			(void)s;

#ifdef __cpp_lib_ranges_zip
			std::ranges::for_each(
				std::views::zip(declaration().parameters(), arguments),
				[&env=inter.current_env()](auto&& t)
				{
					auto name = std::get<0>(t).lexeme();
					auto const& value{std::get<1>(t)};
					env.define(name, value);
				}
			);
#else
			auto& env = inter.current_env();
			auto const& params = declaration().parameters();
			for (size_t i = 0; i < arity(); ++i)
			{
				auto name{std::string{params[i].lexeme()}};
				auto value{arguments[i]};
				env.define(name, std::move(value));
			}
#endif

			try
			{
				inter.execute_block(declaration().body());
			}
			catch(interpreter::return_value const& rv)
			{
				return rv.value_;
			}

			return {};
		}

		std::string name() const override
		{
			return std::string{declaration().name().lexeme()};
		}

		std::string str() const override
		{ return fmt::format("<fn {}>", name()); }

	private:
		func_stmt declaration_;
	};


	callable callable::make_lox_function(func_stmt const& declaration)
	{
		return callable{std::make_shared<lox_function_impl>(declaration)};
	}

}

