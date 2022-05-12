#include <iterator>
#include <ranges>
#include <sstream>
#include "lox/callable.hpp"
#include "lox/object.hpp"
#include "lox/builtins/clock.hpp"
#include "lox/builtins/dir.hpp"
#include "lox/interpreter.hpp"


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
		object clock::call(interpreter& inter, std::vector<object> const& args)
		{
			ignore_unused(inter, args);

			auto now{std::chrono::high_resolution_clock::now()};
			auto tp{now.time_since_epoch()};
			
			using seconds = std::chrono::duration<double>;
			return object{std::chrono::duration_cast<seconds>(tp).count()};
		}

		object dir::call(interpreter& inter, std::vector<object> const& args)
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
}

