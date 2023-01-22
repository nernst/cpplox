#include "test_utils.hpp"

BOOST_AUTO_TEST_CASE(simple_instance_test)
{
    const auto test = R"test(
class Brioche{}
print Brioche();
    )test"s;
    const auto expected = "Brioche instance"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(simple_set_test)
{
    const auto test = R"test(
class Class{}
var instance = Class();
print instance.prop = "pass";
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(simple_set_get_test)
{
    const auto test = R"test(
class Class{}
var instance = Class();
instance.prop = "pass";
print instance.prop;
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}
