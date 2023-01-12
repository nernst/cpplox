#include "test_utils.hpp"

BOOST_AUTO_TEST_CASE(simple_if)
{
    const auto test = R"test(
if (1)
    print "pass";
)test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);

    if (res != lox::VM::Result::OK)
    {
        std::cerr << err << std::endl;
    }

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(simple_if_else)
{
    const auto test = R"test(
if (false)
    print "fail";
else
    print "pass";
)test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(and_test)
{
    const auto test = R"test(
if (false and true)
    print "fail1";
else if (true and false)
    print "fail2";
else if (true and true)
    print "pass";
else
    print "fail3";
)test"s;
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(or_test)
{
    const auto test = R"test(
if (false or true) {
    print "pass";
} else {
    print "fail1";
}

if (true or false)
    print "pass";
else
    print "fail2";

if (true or true)
    print "pass";
else
    print "fail3";

if (false or false)
    print "fail4";
else
    print "pass";

)test"s;
    const auto expected = "pass\npass\npass\npass\n"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(while_test)
{
    const auto test = R"test(
var a = 1;
while (a > 0)
{
    print "pass";
    a = a - 1;
}
)test";
    const auto expected = "pass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(for_test)
{
    const auto test = R"test(
for (var a = 1; a <= 5; a = a + 1)
    print a;
    )test"s;
    const auto expected = "1\n2\n3\n4\n5"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(greater_test)
{
    const auto test = R"test(
if (2 > 1)
    print "pass";
else
    print "fail";

if (1 > 2)
    print "fail";
else
    print "pass";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(greater_equal_test)
{
    const auto test = R"test(
if (2 >= 1)
    print "pass";
else
    print "fail";

if (1 >= 2)
    print "fail";
else
    print "pass";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(less_test)
{
    const auto test = R"test(
if (1 < 2)
    print "pass";
else
    print "fail";

if (2 < 1)
    print "fail";
else
    print "pass";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}


BOOST_AUTO_TEST_CASE(less_equal_test)
{
    const auto test = R"test(
if (1 <= 2)
    print "pass";
else
    print "fail";

if (1 <= 2)
    print "pass";
else
    print "fail";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    // std::cerr << err << std::endl;

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(equal_test)
{
    const auto test = R"test(
if (1 == 1)
    print "pass";
else
    print "fail";

if (1 == 2)
    print "fail";
else
    print "pass";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

BOOST_AUTO_TEST_CASE(not_equal_test)
{
    const auto test = R"test(
if (1 != 1)
    print "fail";
else
    print "pass";

if (1 != 2)
    print "pass";
else
    print "fail";
)test";
    const auto expected = "pass\npass"s;

    auto [res, out, err] = run_test(test);

    assert_equal(out, expected);
}

