#include <tuple>
#include <sstream>
#include <boost/test/unit_test.hpp>

#include "lox/lox.hpp"
#include "lox/utility.hpp"

using namespace lox;
using namespace std::literals::string_literals;

template<typename T, typename U>
inline std::tuple<bool, bool, bool, std::string, std::string> run_test_case_output(T&& name, U&& source)
{
	std::istringstream stdin;
	std::ostringstream stdout, stderr;
	string_source s{std::forward<T>(name), std::forward<U>(source)};
	Lox intrpr{&stdin, &stdout, &stderr};
	intrpr.run(s);

	return std::make_tuple(intrpr.had_error(), intrpr.had_parse_error(), intrpr.had_runtime_error(), stdout.str(), stderr.str());
}

template<typename T, typename U>
inline std::tuple<bool, bool, bool> run_test_case(T&& name, U&& source)
{
	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output(std::forward<T>(name), std::forward<U>(source));
	ignore_unused(output, error);

	return std::make_tuple(had_error, had_parse_error, had_runtime_error);
}

BOOST_AUTO_TEST_CASE(interpreter_assign)
{
	auto test = R"test(
var a = 1;
a = 2;
)test";
	auto [had_error, had_parse_error, had_runtime_error] = run_test_case("assign", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
}

BOOST_AUTO_TEST_CASE(interpreter_assign_undefined)
{
	auto test = R"test(
var a = 1;
b = 2;
)test";
	auto [had_error, had_parse_error, had_runtime_error] = run_test_case("assign", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(had_runtime_error);
}

BOOST_AUTO_TEST_CASE(interpreter_scope)
{
	auto test = R"test(
var a = "global a";
var b = "global b";
var c = "global c";
{
	var a = "outer a";
	var b = "outer b";
	{
		var a = "inner a";
		print a;
		print b;
		print c;
	}
	print "";
	print a;
	print b;
	print c;
}
print "";
print a;
print b;
print c;
)test"s;

	auto expected = R"expected(inner a
outer b
global c

outer a
outer b
global c

global a
global b
global c
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("scope", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(output, expected);
}


BOOST_AUTO_TEST_CASE(interpreter_if)
{
	auto test = R"test(
var a = nil;
if (true)
	a = true;
else
	a = false;
print a;
)test"s;

	auto expected = R"expected(true
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("if", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(output, expected);
}


BOOST_AUTO_TEST_CASE(interpreter_if_else)
{
	auto test = R"test(
var a = nil;
if (false)
	a = true;
else
	a = false;
print a;
)test"s;

	auto expected = R"expected(false
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("if-else", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(output, expected);
}

