#include "lox/compiler.hpp"
#ifdef DEBUG_PRINT_CODE
#include "lox/debug.hpp"
#endif
#include "lox/scanner.hpp"
#include <iterator>
#include <sstream>
#include <unordered_map>
#include <fmt/format.h>

namespace lox
{
    namespace
    {
        class Compiler
        {
        public:
            Compiler(std::string const& source)
            : source_{&source}
            , scanner_{source}
            , had_error_{false}
            , panic_mode_{false}
            {}

            bool compile()
            {
                advance();
                while (!match(Token::TOKEN_EOF))
                {
                    declaration();
                }
                end();
                return !had_error_;
            }
            Chunk& chunk() { return chunk_; }

        private:
            std::string const* source_;
            Scanner scanner_;
            Chunk chunk_;
            Token current_;
            Token previous_;
            bool had_error_;
            bool panic_mode_;

            enum class Precedence {
                NONE,
                ASSIGNMENT, // =
                OR,         // or
                AND,        // and
                EQUALITY,   // == !=
                COMPARISON, // < > <= >=
                TERM,       // + -
                FACTOR,     // * /
                UNARY,      // ! -
                CALL,       // . ()
                PRIMARY
            };
            
            void error_at_current(std::string const& message)
            {
                error_at(current_, message);
            }

            void error(std::string const& message)
            {
                error_at(previous_, message);
            }

            void print_error_location(Token const& token)
            {
                std::string_view source{*source_};
                const auto token_start = std::distance(source_->data(), token.start);
                auto line_start = source.rfind("\n", token_start);
                if (line_start == std::string_view::npos)
                    line_start = 0;
                auto line_end = source.find("\n", token_start);
                if (line_end == std::string_view::npos)
                    line_end = source.size();

                const auto line = source.substr(line_start, line_end - line_start);
                const auto line_pos = token_start - line_start;
                std::string padding(6 + line_pos, ' ');
                // fmt::print(stderr, "line_start: {}, line_end: {}, line_pos: {}\nline: {}\n", line_start, line_end, line_pos, line);
                fmt::print(stderr, "{:4}: {}\n", token.line, line);
                fmt::print(stderr, "{}^\n", padding);
            }

            void error_at(Token const& token, std::string const& message)
            {
                if (panic_mode_)
                    return;

                panic_mode_ = true;
                had_error_ = true;
                print_error_location(token);

                fmt::print(stderr, "Error", token.line);

                if (token.type == Token::TOKEN_EOF) {
                    fmt::print(stderr, " at end");
                }
                else if (token.type != Token::ERROR) {
                    fmt::print(stderr, " at '{}'", token.token);
                }

                fmt::print(stderr, ": {}\n", message);
            }

            byte make_constant(Value value)
            {
                auto constant = chunk_.add_constant(value);
                if (constant > std::numeric_limits<byte>::max()) {
                    error("Too many constants in one chunk.");
                    return 0;
                }

                return static_cast<byte>(constant);
            }

            void emit(byte value) { chunk_.write(value, previous_.line); }
            void emit(OpCode op) { emit(static_cast<byte>(op)); }
            void emit(OpCode op, byte value) { emit(op); emit(value); }
            void emit_return() { emit(OpCode::OP_RETURN); }
            void emit(Value value) {
                const byte id{make_constant(value)};
                // fmt::print("emit: constant - {}\n", id);
                emit(OpCode::OP_CONSTANT, id); 
            }

            void advance()
            {
                previous_ = current_;
                while (true)
                {
                    current_ = scanner_.scan();
                    if (current_.type != Token::ERROR)
                        break;

                    error_at_current(current_.message); 
                }
            }

            void consume(Token::Type type, std::string const& message)
            {
                if (current_.type == type)
                    advance();
                else
                    error_at_current(message);
            }

            bool check(Token::Type type)
            {
                return current_.type == type;
            }

            bool match(Token::Type type)
            {
                if (!check(type))
                    return false;
                
                advance();
                return true;
            }

            void end()
            { 
                emit_return(); 

#ifdef DEBUG_PRINT_CODE
                if (!had_error_)
                    disassemble(chunk_, "code");
#endif
            }

            void expression()
            {
                parse_precedence(Precedence::ASSIGNMENT);
            }

            void var_declaration()
            {
                byte global = parse_variable("Expect variable name.");

                if (match(Token::EQUAL))
                {
                    expression();
                }
                else
                {
                    emit(OpCode::OP_NIL);
                }
                consume(Token::SEMICOLON, "Expect ';' after variable declaration.");

                define_variable(global);
            }

            void expression_statement()
            {
                expression();
                consume(Token::SEMICOLON, "Expect ';' after expression.");
                emit(OpCode::OP_POP);
            }

            void print_statement()
            {
                expression();
                consume(Token::SEMICOLON, "Expect ';' after value.");
                emit(OpCode::OP_PRINT);
            }

            void synchronize()
            {
                panic_mode_ = false;

                while (current_.type != Token::TOKEN_EOF)
                {
                    if (previous_.type == Token::SEMICOLON)
                        return;

                    switch (current_.type)
                    {
                        case Token::CLASS:
                        case Token::FUN:
                        case Token::VAR:
                        case Token::FOR:
                        case Token::IF:
                        case Token::WHILE:
                        case Token::PRINT:
                        case Token::RETURN:
                            return;
                        
                        default:
                            ;
                    }

                    advance();
                }
            }

            void declaration()
            {
                if (match(Token::VAR))
                {
                    var_declaration();
                }
                else
                {
                    statement();
                }

                if (panic_mode_)
                    synchronize();
            }

            void statement()
            {
                if (match(Token::PRINT))
                {
                    print_statement();
                }
                else
                {
                    expression_statement();
                }
            }

            void number()
            {
                std::stringstream ss{std::string(previous_.token)};
                double d;
                ss >> d;

                emit(Value{d});
            }

            void string()
            {
                emit(Value{
                    new String{std::string{previous_.start + 1, previous_.length - 2}}
                });
            }

            void named_variable(Token const& name)
            {
                byte arg = identifier_constant(name);
                emit(OpCode::OP_GET_GLOBAL, arg);
            }

            void variable()
            {
                named_variable(previous_);
            }

            void grouping()
            {
                expression();
                consume(Token::RIGHT_PAREN, "Expect ')' after expression.");
            }

            void unary()
            {
                Token::Type op{previous_.type};

                parse_precedence(Precedence::UNARY);

                switch(op)
                {
                    case Token::MINUS: emit(OpCode::OP_NEGATE); break;
                    case Token::BANG: emit(OpCode::OP_NOT); break;
                    default: unreachable();
                }
            }

            void binary()
            {
                Token::Type op{previous_.type};
                ParseRule const* rule = get_rule(op);
                assert(rule != nullptr);

                parse_precedence(static_cast<Precedence>(static_cast<int>(rule->precedence) + 1));

                switch(op)
                {
                    case Token::PLUS: emit(OpCode::OP_ADD); break;
                    case Token::MINUS: emit(OpCode::OP_SUBTRACT); break;
                    case Token::STAR: emit(OpCode::OP_MULTIPLY); break;
                    case Token::SLASH: emit(OpCode::OP_DIVIDE); break;
                    case Token::BANG_EQUAL: emit(OpCode::OP_EQUAL); emit(OpCode::OP_NOT); break;
                    case Token::EQUAL_EQUAL: emit(OpCode::OP_EQUAL); break;
                    case Token::GREATER: emit(OpCode::OP_GREATER); break;
                    case Token::GREATER_EQUAL: emit(OpCode::OP_LESS); emit(OpCode::OP_NOT); break;
                    case Token::LESS: emit(OpCode::OP_LESS); break;
                    case Token::LESS_EQUAL: emit(OpCode::OP_GREATER); emit(OpCode::OP_NOT); break;

                    default: unreachable();
                }
            }

            void literal()
            {
                switch(previous_.type)
                {
                    case Token::FALSE: emit(OpCode::OP_FALSE); break;
                    case Token::NIL: emit(OpCode::OP_NIL); break;
                    case Token::TRUE: emit(OpCode::OP_TRUE); break;
                    default: unreachable();
                }
            }

            byte identifier_constant(Token const& name)
            {
                return make_constant(Value{new String(name.token)});
            }

            byte parse_variable(const char* error_message)
            {
                consume(Token::IDENTIFIER, error_message);
                return identifier_constant(previous_);
            }

            void define_variable(byte global) {
                emit(OpCode::OP_DEFINE_GLOBAL, global);
            }

            void parse_precedence(Precedence precedence)
            {
                advance();
                ParseRule::Func prefix = get_rule(previous_.type)->prefix;

                if (!prefix)
                {
                    error("Expect expression.");
                    return;
                }

                (this->*prefix)();

                while (precedence <= get_rule(current_.type)->precedence)
                {
                    advance();

                    ParseRule::Func infix = get_rule(previous_.type)->infix;
                    (this->*infix)();
                }
            }

            struct ParseRule
            {
                using Func = void(Compiler::*)();
                Func prefix;
                Func infix;
                Precedence precedence;
            };

            ParseRule const* get_rule(Token::Type op)
            {
                static const std::unordered_map<Token::Type, ParseRule> rules = {
                    #define RULE(token, prefix, infix, precedence) \
                        {Token::token, {prefix, infix, Precedence::precedence }}
                    RULE(LEFT_PAREN, &Compiler::grouping, nullptr, NONE),
                    RULE(MINUS, &Compiler::unary, &Compiler::binary, TERM),
                    RULE(PLUS, nullptr, &Compiler::binary, TERM),
                    RULE(SLASH, nullptr, &Compiler::binary, FACTOR),
                    RULE(STAR, nullptr, &Compiler::binary, FACTOR),
                    RULE(NUMBER, &Compiler::number, nullptr, NONE),
                    RULE(IDENTIFIER, &Compiler::variable, nullptr, NONE),
                    RULE(STRING, &Compiler::string, nullptr, NONE),
                    RULE(FALSE, &Compiler::literal, nullptr, NONE),
                    RULE(TRUE, &Compiler::literal, nullptr, NONE),
                    RULE(NIL, &Compiler::literal, nullptr, NONE),
                    RULE(BANG, &Compiler::unary, nullptr, NONE),
                    RULE(BANG_EQUAL, nullptr, &Compiler::binary, EQUALITY),
                    RULE(EQUAL_EQUAL, nullptr, &Compiler::binary, EQUALITY),
                    RULE(GREATER, nullptr, &Compiler::binary, COMPARISON),
                    RULE(GREATER_EQUAL, nullptr, &Compiler::binary, COMPARISON),
                    RULE(LESS, nullptr, &Compiler::binary, COMPARISON),
                    RULE(LESS_EQUAL, nullptr, &Compiler::binary, COMPARISON),

                    #undef RULE
                };
                static const ParseRule default_rule{nullptr, nullptr, Precedence::NONE};

                auto iter = rules.find(op);
                if (iter == std::cend(rules))
                    return &default_rule;
                return &(iter->second);
            }

        };

    }
    bool compile(std::string const& source, Chunk& chunk)
    {
        Compiler compiler{source};
        bool success = compiler.compile();
        if (success)
            chunk = std::move(compiler.chunk());

#ifdef DEBUG_COMPILE
        fmt::print(stderr, "compile rc = {}\n", success);
#endif
        return success;
    }
}