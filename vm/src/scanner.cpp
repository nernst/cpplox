#include "lox/scanner.hpp"
#include <cstring>

namespace lox
{
    Scanner::Scanner(std::string const& source)
    : start_{source.data()}
    , current_{source.data()}
    , end_{source.data() + source.size()}
    , line_{1}
    { }

    Token Scanner::scan()
    {
        skip_ws();
        start_ = current_;

        if (is_at_end()) return make(Token::TOKEN_EOF);

        char c = advance();

        if (is_alpha(c))
            return identifier();

        if (is_digit(c))
            return number();

        switch(c)
        {
            case '(': return make(Token::LEFT_PAREN);
            case ')': return make(Token::RIGHT_PAREN);
            case '{': return make(Token::LEFT_BRACE);
            case '}': return make(Token::RIGHT_BRACE);
            case ';': return make(Token::SEMICOLON);
            case ',': return make(Token::COMMA);
            case '.': return make(Token::DOT);
            case '-': return make(Token::MINUS);
            case '+': return make(Token::PLUS);
            case '/': return make(Token::SLASH);
            case '*': return make(Token::STAR);

            case '!': return make(match('=') ? Token::BANG_EQUAL : Token::BANG);
            case '=': return make(match('=') ? Token::EQUAL_EQUAL : Token::EQUAL);
            case '<': return make(match('=') ? Token::LESS_EQUAL : Token::LESS);
            case '>': return make(match('=') ? Token::GREATER_EQUAL : Token::GREATER);

            case '"': return string();

            default: break;
        }

        return error("Unexpected character: '{}'.", c);
    }

    void Scanner::skip_ws()
    {
        while (true)
        {
            switch(peek())
            {
                case ' ':
                case '\r':
                    advance();
                    break;

                case '\n':
                    ++line_;
                    advance();
                    break;

                case '/':
                    // handle comments
                    if (peek_next() == '/')
                    {
                        while (peek() != '\n' && !is_at_end())
                            advance();
                    }
                    else
                        return;

                    break;
                
                default:
                    return;
            }
        }
    }

    Token Scanner::string()
    {
        while (peek() != '"' && !is_at_end())
        {
            if (peek() == '\n')
                ++line_;
            advance();
        }

        if (is_at_end())
            return error("Unterminated string.");

        advance();
        return make(Token::STRING);
    }

    Token Scanner::number()
    {
        while (is_digit(peek()))
            advance();
        
        // fractional part
        if (peek() == '.' && is_digit(peek_next()))
        {
            advance(); // consume '.'
            while (is_digit(peek()))
                advance();
        }
        return make(Token::NUMBER);
    }

    Token Scanner::identifier()
    {
        while (is_alpha(peek()) || is_digit(peek()))
            advance();

        return make(identifier_type());
    }

    Token::Type Scanner::identifier_type() const
    {
        switch(*start_)
        {
            case 'a': return check_keyword(1, 2, "nd", Token::AND);
            case 'c': return check_keyword(1, 4, "lass", Token::CLASS);
            case 'e': return check_keyword(1, 3, "lse", Token::ELSE);
            case 'i': return check_keyword(1, 1, "f", Token::IF);
            case 'n': return check_keyword(1, 2, "il", Token::NIL);
            case 'o': return check_keyword(1, 1, "r", Token::OR);
            case 'p': return check_keyword(1, 4, "rint", Token::PRINT);
            case 'r': return check_keyword(1, 5, "eturn", Token::RETURN);
            case 's': return check_keyword(1, 4, "uper", Token::SUPER);
            case 'v': return check_keyword(1, 2, "ar", Token::VAR);
            case 'w': return check_keyword(1, 4, "hile", Token::WHILE);
            case 'f':
                if (current_ - start_ > 1)
                {
                    switch(start_[1])
                    {
                        case 'a': return check_keyword(2, 3, "lse", Token::FALSE);
                        case 'o': return check_keyword(2, 1, "r", Token::FOR);
                        case 'u': return check_keyword(2, 1, "n", Token::FUN);
                    }
                }
                break;

            case 't':
                if (current_ - start_ > 1)
                {
                    switch(start_[1])
                    {
                        case 'h': return check_keyword(2, 2, "is", Token::THIS);
                        case 'r': return check_keyword(2, 2, "ue", Token::TRUE);
                    }
                }
                break;
            default:
                break;
        }
        return Token::IDENTIFIER;
    }

    Token::Type Scanner::check_keyword(size_t start, size_t length, const char* rest, Token::Type type) const
    {
        if (static_cast<size_t>(current_ - start_) == start + length 
            && std::memcmp(start_ + start, rest, length) == 0)
            return type;

        return Token::IDENTIFIER;
    }
}
