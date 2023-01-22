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
