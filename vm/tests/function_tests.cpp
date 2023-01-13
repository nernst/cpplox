#include "test_utils.hpp"

BOOST_AUTO_TEST_CASE(fun_no_args_test)
{
    auto test = R"test(

fun test_fun() {
    print "fail";
}
print test_fun;

    )test"s;

    auto expected = "<fn test_fun>"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(fun_args_test)
{
    auto test = R"test(

fun test_fun(a, b, c) {
    print "fail";
}
print test_fun;

    )test"s;

    auto expected = "<fn test_fun>"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}
