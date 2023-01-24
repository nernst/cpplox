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

BOOST_AUTO_TEST_CASE(class_simple_method_test)
{
    const auto test = R"test(
class Class{
    test(first, second) { print first + second; }
}
var instance = Class();
instance.test("pa", "ss");
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(class_property_test)
{
    const auto test = R"test(
class Class{}
var instance = Class();
instance.property = "pass";
print instance.property;
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(class_init_test)
{
    const auto test = R"test(
class Class{
    init(message)
    {
        this.message = message;
    }

    say()
    { print this.message; }
}
var instance = Class("pass");
instance.say();
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(class_non_method_invoke_test)
{
    const auto test = R"test(
class Class{
    init()
    {
        fun f(){ print "pass"; }
        this.test = f;
    }
}
var instance = Class();
instance.test();
    )test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(class_super_invoke_test)
{
    const auto test = R"test(
class Base{
    test()
    {
        print "pass";
    }

    say()
    {
        this.test();
    }
}
class Derived < Base{
    test() {
        print "fail";
    }

    say() {
        super.test();
    }
}

Base().say();
Derived().say();
    )test"s;
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);
    assert_equal(res, lox::VM::Result::OK);
    assert_equal(out, expected);
}
