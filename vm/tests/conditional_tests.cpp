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

    if (res != lox::VM::Result::OK)
    {
        std::cerr << err << std::endl;
    }

    assert_equal(out, expected);
}
