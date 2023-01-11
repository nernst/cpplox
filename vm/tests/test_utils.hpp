#include <string>
#include <iostream>
#include <sstream>
#include <tuple>
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

    return std::make_tuple(result, out.str(), err.str());
}
