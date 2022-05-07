#include <tuple>
#include <sstream>
#include <boost/test/unit_test.hpp>

#include "lox/lox.hpp"

using namespace lox;

template<typename T, typename U>
inline std::tuple<bool, bool, bool> run_test_case(T&& name, U&& source)
{
	std::istringstream stdin;
	std::ostringstream stdout, stderr;
	string_source s{std::forward<T>(name), std::forward<U>(source)};
	Lox intrpr{&stdin, &stdout, &stderr};
	intrpr.run(s);

	return std::make_tuple(intrpr.had_error(), intrpr.had_parse_error(), intrpr.had_runtime_error());
}

BOOST_AUTO_TEST_CASE(interpreter_assign)
{
	auto test = R"test(
var a = 1;
a = 2;
)test";
	string_source s{"assign", test};

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
	string_source s{"assign_undefined", test};

	auto [had_error, had_parse_error, had_runtime_error] = run_test_case("assign", test);

	BOOST_TEST(!had_error);
	BOOST_TEST(!had_parse_error);
	BOOST_TEST(had_runtime_error);
}

