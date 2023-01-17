#include "test_utils.hpp"

BOOST_AUTO_TEST_CASE(fun_no_args_test)
{
    const auto test = R"test(

fun test_fun() {
    print "fail";
}
print test_fun;

    )test"s;

    const auto expected = "<fn test_fun>"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(fun_args_test)
{
    const auto test = R"test(

fun test_fun(a, b, c) {
    print "fail";
}
print test_fun;

    )test"s;

    const auto expected = "<fn test_fun>"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(fun_call_no_args_test)
{
    const auto test = R"test(
fun test_fun() {
    print "pass";
}
test_fun();
    )test"s;

    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(fun_call_args_test)
{
    const auto test = R"test(
fun test_fun(a, b) {
    print a + b;
}
test_fun("pa", "ss");
    )test"s;

    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(func_call_no_args_return_test)
{
    const auto test = R"test(
fun function() {
    return "pass";
}
print function();
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(func_call_args_return_test)
{
    const auto test = R"test(
fun function(a, b) {
    return a + b;
}
print function("pa", "ss");
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(func_reference_outer_scope_test)
{
    const auto test = R"test(
fun outer() {
    var x = "pass";
    fun inner() {
        print x;
    }
    return inner;
}
var closure = outer();
closure();
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(func_simple_closure_test)
{
    const auto test = R"test(
fun outer(a)
{
    var b = "world";
    fun inner(c)
    {
        print a + " " + b + " " + c + "!";
    }
    return inner;

}
var closure = outer("hello");
closure("from lox");
    )test"s;
    const auto expected = "hello world from lox!"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

