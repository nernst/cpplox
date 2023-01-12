#include <string>
#include <iostream>
#include <sstream>
#include <tuple>
#include <boost/test/unit_test.hpp>
#include "lox/vm.hpp"

using namespace std::literals::string_literals;

namespace lox {
    inline std::ostream& operator<<(std::ostream& os, lox::VM::Result res)
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

inline std::tuple<lox::VM::Result, std::string, std::string> run_test(std::string const& source)
{
    std::stringstream out, err;
    lox::VM vm{out, err};
    auto result = vm.interpret(source);

    if (result != lox::VM::Result::OK)
    {
        std::cerr << err.str() << std::endl;
    }

    return std::make_tuple(result, out.str(), err.str());
}

template<typename T>
std::decay_t<T> strip_if_string(T&& value)
{
    if constexpr(std::is_same_v<std::decay_t<T>, std::string>)
        return lox::strip(std::forward<T>(value));
    else
        return std::forward<T>(value);
}

template<typename T, typename U>
void assert_equal(T&& lhs, U&& rhs)
{
    auto l = strip_if_string(std::forward<T>(lhs));
    auto r = strip_if_string(std::forward<U>(rhs));
    BOOST_TEST(l == r);
}

template<typename T, typename U>
void assert_not_equal(T&& lhs, U&& rhs)
{
    BOOST_TEST(strip_if_string(std::forward<T>(lhs)) != strip_if_string(std::forward<U>(rhs)));
}
