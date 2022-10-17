#include <boost/test/unit_test.hpp>
#include "lox/value.hpp"

using namespace lox;

BOOST_AUTO_TEST_CASE(value_bool_tests)
{
    BOOST_TEST(Value{}.is_false());
    BOOST_TEST(Value{true}.is_true());
    BOOST_TEST(Value{false}.is_false());
    BOOST_TEST(Value{0}.is_true());
    BOOST_TEST(Value{1}.is_true());
}
