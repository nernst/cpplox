#include "lox/compiler.hpp"
#include "lox/scanner.hpp"
#include <fmt/format.h>

namespace lox
{
    Chunk compile(std::string const& source)
    {
        Scanner scanner{source};
        size_t line{static_cast<size_t>(-1)};

        while (true)
        {
            auto token{scanner.scan()};

            if (token.line != line)
            {
                fmt::print("{:4d} ", token.line);
                line = token.line;
            }
            else
            {
                fmt::print("   | ");
            }

            fmt::print("{:2d} '{}'\n", token.type, std::string_view{token.start, token.length});
            if (token.type == Token::TOKEN_EOF)
                break;
        }

        return {};
    }
}