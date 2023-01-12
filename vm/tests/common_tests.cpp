#include "test_utils.hpp"

using lox::strip;

BOOST_AUTO_TEST_CASE(strip_test)
{
    BOOST_TEST("asdf"s == strip("  asdf"s));
    BOOST_TEST("asdf"s == strip("\t\nasdf"s));
    BOOST_TEST("asdf"s == strip("asdf\t\n  "s));
    BOOST_TEST("asdf"s == strip("   \t\nasdf\t\n  "s));
    BOOST_TEST("asdf"s == strip("asdf"s));
}
