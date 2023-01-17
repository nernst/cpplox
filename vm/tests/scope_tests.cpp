#include "test_utils.hpp"

BOOST_AUTO_TEST_CASE(variable_shadowing_test)
{
    const auto test = R"test(
var a = 1;
print a;
{
    var a = 2;
    print a;
}
print a;
)test"s;
    const auto expected = "1\n2\n1\n"s;

    auto [result, out, err] = run_test(test);

    BOOST_TEST(result == lox::VM::Result::OK);
    assert_equal(out, expected);
#if !defined(DEBUG_COMPILE) && !defined(DEBUG_PRINT_CODE) && !defined(DEBUG_TRACE_EXECUTION)
    assert_equal(err, ""s);
#endif
}
