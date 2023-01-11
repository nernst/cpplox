#include "lox/compiler.hpp"
#ifdef DEBUG_PRINT_CODE
#include "lox/debug.hpp"
#endif
#include "lox/scanner.hpp"
#include <array>
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
            struct Local
            {
                Token name;
                int depth;
            };

        public:
            Compiler(std::string const& source)
            : source_{&source}
            , scanner_{source}
            , had_error_{false}
            , panic_mode_{false}
            , local_count_{0}
            , scope_depth_{0}
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

            static constexpr size_t max_locals = static_cast<size_t>(std::numeric_limits<byte>::max()) + 1;

            std::array<Local, max_locals> locals_;
            int local_count_;
            int scope_depth_;

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

            void begin_scope()
            {
                ++scope_depth_;
            }

            void end_scope()
            {
                --scope_depth_;

                while (local_count_ > 0 && locals_[local_count_ - 1].depth > scope_depth_)
                {
                    emit(OpCode::OP_POP);
                    --local_count_;
                }
            }

            void expression()
            {
                parse_precedence(Precedence::ASSIGNMENT);
            }

            void block()
            {
                while (!check(Token::RIGHT_BRACE) && !check(Token::TOKEN_EOF))
                {
                    declaration();
                }
                consume(Token::RIGHT_BRACE, "Expect '}' after block.");
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
                else if (match(Token::LEFT_BRACE))
                {
                    begin_scope();
                    block();
                    end_scope();
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

            void number(bool can_assign)
            {
                ignore(can_assign);

                std::stringstream ss{std::string(previous_.token)};
                double d;
                ss >> d;

                emit(Value{d});
            }

            void string(bool can_assign)
            {
                ignore(can_assign);

                emit(Value{
                    new String{std::string{previous_.start + 1, previous_.length - 2}}
                });
            }

            void named_variable(Token const& name, bool can_assign)
            {
                OpCode get_op, set_op;
                int arg = resolve_local(name);
                if (arg != -1)
                {
                    get_op = OpCode::OP_GET_LOCAL;
                    set_op = OpCode::OP_SET_LOCAL;
                }
                else
                {
                    get_op = OpCode::OP_GET_GLOBAL;
                    set_op = OpCode::OP_SET_GLOBAL;
                }

                if (can_assign && match(Token::EQUAL))
                {
                    expression();
                    emit(set_op, static_cast<byte>(arg & 0xff));
                }
                else
                {
                    emit(get_op, static_cast<byte>(arg & 0xff));
                }
            }

            void variable(bool can_assign)
            {
                named_variable(previous_, can_assign);
            }

            void grouping(bool can_assign)
            {
                ignore(can_assign);

                expression();
                consume(Token::RIGHT_PAREN, "Expect ')' after expression.");
            }

            void unary(bool can_assign)
            {
                ignore(can_assign);

                Token::Type op{previous_.type};

                parse_precedence(Precedence::UNARY);

                switch(op)
                {
                    case Token::MINUS: emit(OpCode::OP_NEGATE); break;
                    case Token::BANG: emit(OpCode::OP_NOT); break;
                    default: unreachable();
                }
            }

            void binary(bool can_assign)
            {
                ignore(can_assign);

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

            void literal(bool can_assign)
            {
                ignore(can_assign);

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

            static bool identifiers_equal(Token const& lhs, Token const& rhs)
            {
                return lhs.token == rhs.token;
            }

            int resolve_local(Token const& name)
            {
                for (int i = local_count_ - 1; i >= 0; --i)
                {
                    if (identifiers_equal(name, locals_[i].name))
                    {
                        if (locals_[i].depth == -1)
                        {
                            error("Cannot read local variable in its own initializer.");
                        }
                        return i;
                    }
                }

                return -1;
            }

            void add_local(Token const& name)
            {
                if (local_count_ == max_locals)
                {
                    error("Too many local variables in function.");
                    return;
                }

                Local& local = locals_[local_count_++];
                local.name = name;
                local.depth = -1;
            }

            void declare_variable()
            {
                if (scope_depth_ == 0)
                    return;
                
                Token name{previous_};
                for (int i = local_count_ - 1; i >= 0; --i)
                {
                    auto& local = locals_[i];
                    if (local.depth != -1 && local.depth < scope_depth_)
                        break;

                    if (identifiers_equal(local.name, name))
                    {
                        error("Already a variable with this name in this scope.");
                    }
                }

                add_local(std::move(name));
            }

            byte parse_variable(const char* error_message)
            {
                consume(Token::IDENTIFIER, error_message);
                declare_variable();
                if (scope_depth_ > 0)
                    return 0;

                return identifier_constant(previous_);
            }

            void mark_initialized()
            {
                locals_[local_count_ - 1].depth = scope_depth_;
            }

            void define_variable(byte global)
            {
                if (scope_depth_ > 0)
                {
                    mark_initialized();
                    return;
                }

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

                bool can_assign = precedence <= Precedence::ASSIGNMENT;
                (this->*prefix)(can_assign);

                while (precedence <= get_rule(current_.type)->precedence)
                {
                    advance();

                    ParseRule::Func infix = get_rule(previous_.type)->infix;
                    (this->*infix)(can_assign);
                }

                if (can_assign && match(Token::EQUAL))
                    error("Invalid assignment target.");
            }

            struct ParseRule
            {
                using Func = void(Compiler::*)(bool);
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