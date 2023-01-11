#include "lox/vm.hpp"
#include <string>
#include <boost/test/unit_test.hpp>

using namespace std::literals::string_literals;

namespace lox {
    std::ostream& operator<<(std::ostream& os, lox::VM::Result res)
    {
        switch(res)
        {
            case lox::VM::Result::OK: os << "OK"; break;
            case lox::VM::Result::RUNTIME_ERROR: os << "RUNTIME_ERROR"; break;
            case lox::VM::Result::COMPILE_ERROR: os << "COMPILER_ERROR"; break;
            default: os << "!!INVALID Result value: " << static_cast<size_t>(res) << "!!"; break;
        }
        return os;
    }
}

std::tuple<lox::VM::Result, std::string, std::string> run_test(std::string const& source)
{
    std::stringstream out, err;
    lox::VM vm{out, err};
    auto result = vm.interpret(source);

    return std::make_tuple(result, out.str(), err.str());
}

BOOST_AUTO_TEST_CASE(variable_shadowing)
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
    BOOST_TEST(out == expected);
    // BOOST_TEST(err == ""s);
}
