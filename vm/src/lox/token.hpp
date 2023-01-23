#pragma once

#include "common.hpp"

namespace lox
{
    struct Token
    {
        enum Type {
            INVALID,
            LEFT_PAREN,
            RIGHT_PAREN,
            LEFT_BRACE,
            RIGHT_BRACE,
            COMMA,
            DOT,
            MINUS,
            PLUS,
            SEMICOLON,
            SLASH,
            STAR,

            BANG,
            BANG_EQUAL,
            EQUAL,
            EQUAL_EQUAL,
            GREATER,
            GREATER_EQUAL,
            LESS,
            LESS_EQUAL,

            IDENTIFIER,
            STRING,
            NUMBER,

            AND,
            CLASS,
            ELSE,
            FALSE,
            FOR,
            FUN,
            IF,
            NIL,
            OR,
            PRINT,
            RETURN,
            SUPER,
            THIS,
            TRUE,
            VAR,
            WHILE,

            ERROR,
            TOKEN_EOF,
        };

        Token(size_t line, Type type, const char* start, size_t len, std::string message = std::string())
        : line{line}
        , type{type}
        , start{start}
        , length{len}
        , token{start, len}
        , message{move(message)}
        {}

        Token()
        : line{static_cast<size_t>(-1)}
        , type{INVALID}
        , start{nullptr}
        , length{0}
        {}

        explicit Token(std::string_view token)
        : line{static_cast<size_t>(-1)}
        , type{INVALID}
        , start(token.data())
        , length(token.size())
        , token{token}
        {}

        Token(Token const&) = default;
        Token(Token&&) = default;

        Token& operator=(Token const&) = default;
        Token& operator=(Token&&) = default;

        size_t line;
        Type type;
        const char* start;
        size_t length;
        std::string_view token;
        std::string message;
    };
}
