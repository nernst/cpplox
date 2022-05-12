#include "lox/callable.hpp"
#include "lox/object.hpp"
#include "lox/builtins/clock.hpp"


namespace lox
{
	object callable::operator()(interpreter& inter, std::vector<object> const& arguments) const
	{ return impl_->call(inter, arguments); }

	std::vector<callable> builtins()
	{
		return std::vector<callable>{{
			{ callable::make<builtin::clock>() },
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
	}
}

