#include <tuple>
#include <sstream>
#include <boost/test/unit_test.hpp>

#include "lox/lox.hpp"
#include "lox/utility.hpp"

using namespace lox;
using namespace std::literals::string_literals;

template<typename T, typename U>
inline std::tuple<bool, bool, bool, std::string, std::string> run_test_case_output(T&& name, U&& source, bool trim_output=true)
{
	std::istringstream stdin;
	std::ostringstream stdout, stderr;
	string_source s{std::forward<T>(name), std::forward<U>(source)};
	Lox intrpr{&stdin, &stdout, &stderr};
	intrpr.run(s);

	auto out = stdout.str();
	auto err = stderr.str();

	if (trim_output)
	{
		out = trim(out);
		err = trim(err);
	}

	return std::make_tuple(intrpr.had_error(), intrpr.had_parse_error(), intrpr.had_runtime_error(), out, err);
}

template<typename T, typename U>
inline std::tuple<bool, bool, bool> run_test_case(T&& name, U&& source)
{
	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output(std::forward<T>(name), std::forward<U>(source), false);
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
	auto [had_error, had_parse_error, had_runtime_error] = run_test_case("assign-undefined", test);

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
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
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

	auto expected = R"expected(true)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("if", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
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

	auto expected = R"expected(false)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("if-else", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, expected);
}

BOOST_AUTO_TEST_CASE(interpreter_and)
{
	auto test = R"test(
print "test1" and "test2";
print nil and "test3";
)test"s;

	auto expected = R"expected(test2
nil)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("logical-and", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, expected);
}

BOOST_AUTO_TEST_CASE(interpreter_or)
{
	auto test = R"test(
print "test1" or "test2";
print nil or "test3";
)test"s;

	auto expected = R"expected(test1
test3)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("logical-and", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, expected);
}

BOOST_AUTO_TEST_CASE(interpreter_while)
{
	auto test = R"test(
var a = 5;

while (a > 0)
{
	print a;
	a = a - 1;
}

)test"s;

	auto expected = R"expected(
5
4
3
2
1
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("while", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}


BOOST_AUTO_TEST_CASE(interpreter_for)
{
	auto test = R"test(
var a = 0;
var temp;

for (var b = 1; a < 10000; b = temp + b) {
	print a;
	temp = a;
	a = b;
}
)test"s;

	auto expected = R"expected(
0
1
1
2
3
5
8
13
21
34
55
89
144
233
377
610
987
1597
2584
4181
6765
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("for", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}


BOOST_AUTO_TEST_CASE(interpreter_call_not_callable)
{
	auto test = R"test(
var a = 0;
a();
)test"s;

	auto expected = R"expected(
)expected"s;

	try
	{
		(void)run_test_case_output("call-not-callable", test);
	}
	catch(type_error const& e)
	{
		BOOST_REQUIRE_EQUAL(e.what(), "Only functions and classes are callable.");
	}
}

BOOST_AUTO_TEST_CASE(interpreter_simple_func)
{
	auto test = R"test(
fun count(n) {
	if (n > 1) count(n - 1);
	print n;
}

count(3);
)test"s;

	auto expected = R"expected(
1
2
3
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("simple-func", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}

BOOST_AUTO_TEST_CASE(interpreter_func_str)
{
	auto test = R"test(
fun add(a, b) {
	print a + b;
}

print add;
)test"s;

	auto expected = R"expected(
<fn add>
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("func-str", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}

BOOST_AUTO_TEST_CASE(interpreter_func_return)
{
	auto test = R"test(
fun fib(n) {
	if (n <= 1) return n;
	return fib(n - 2) + fib(n - 1);
}

for (var i = 0; i < 20; i = i + 1) {
	print fib(i);
}
)test"s;

	auto expected = R"expected(
0
1
1
2
3
5
8
13
21
34
55
89
144
233
377
610
987
1597
2584
4181
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("func-return", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}

BOOST_AUTO_TEST_CASE(interpreter_closure)
{
	auto test = R"test(
fun makeCounter() {
	var i = 0;
	fun count() {
		i = i + 1;
		print i;
	}

	return count;
}

var counter = makeCounter();
counter(); // 1
counter(); // 2
)test"s;

	auto expected = R"expected(
1
2
)expected"s;

	auto [had_error, had_parse_error, had_runtime_error, output, error] = run_test_case_output("func-return", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(!had_runtime_error);
	BOOST_REQUIRE_EQUAL(error, ""s);
	BOOST_REQUIRE_EQUAL(output, trim(expected));
}


