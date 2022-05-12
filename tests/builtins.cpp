#include <array>
#include <utility>
#include <boost/test/unit_test.hpp>

#include "lox/callable.hpp"
#include "lox/builtins/clock.hpp"
#include "lox/builtins/dir.hpp"
#include "lox/interpreter.hpp"

using namespace lox;

BOOST_AUTO_TEST_CASE(builtin_clock)
{
	std::stringstream in, out, err;
	interpreter inter{&in, &out, &err};
	auto func{callable::make<lox::builtin::clock>()};
	object value{func(inter, std::vector<object>{})};

	BOOST_TEST(static_cast<double>(value) > 0);
}

BOOST_AUTO_TEST_CASE(builtin_dir)
{
	std::stringstream in, out, err;
	interpreter inter{&in, &out, &err};
	auto func{callable::make<lox::builtin::dir>()};
	object value{func(inter, std::vector<object>{})};

	BOOST_TEST(value.str() == "{clock, dir}");
}

