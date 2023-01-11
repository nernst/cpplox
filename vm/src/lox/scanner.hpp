#pragma once

#include "common.hpp"
#include <cassert>
#include <fmt/format.h>

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

    class Scanner
    {
    public:
        explicit Scanner(std::string const& source);

        Token scan();

        bool is_at_end() const {
            return current_ == end_;
        }

    private:
        const char* start_;
        const char* current_;
        const char* end_;
        size_t line_;

        char peek() const {
            return *current_;
        }
        char peek_next() const {
            if (is_at_end()) return '\0';
            return current_[1];
        }

        char advance() {
            assert(!is_at_end());
            return *current_++;
        }

        bool match(char expected) {
            if (is_at_end())
                return false;
            
            if (*current_ != expected)
                return false;
            
            ++current_;
            return true;
        }

        void skip_ws();
        Token string();
        Token number();
        Token identifier();
        Token::Type identifier_type() const;
        Token::Type check_keyword(size_t start, size_t length, const char* rest, Token::Type type) const;

        template<typename... Args>
        Token error(fmt::format_string<Args...> format, Args&&... args) {
            auto length = static_cast<size_t>(current_ - start_);
            return Token{
                line_,
                Token::ERROR,
                start_, 
                length,
                fmt::format(format, std::forward<Args>(args)...)
            };
        }

        Token make(Token::Type type) const {
            auto length = static_cast<size_t>(current_ - start_);
            return Token{
                line_,
                type,
                start_,
                length
            };
        }

        static bool is_digit(char c) {
            return '0' <= c && c <= '9';
        }

        static bool is_alpha(char c) {
            return ('a' <= c && c <= 'z')
                || ('A' <= c && c <= 'Z')
                || c == '_';
        }
    };

}
