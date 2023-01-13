#include "lox/compiler.hpp"
#ifdef DEBUG_PRINT_CODE
#include "lox/debug.hpp"
#endif
#include "lox/scanner.hpp"
#include "lox/object.hpp"
#include <array>
#include <iterator>
#include <sstream>
#include <unordered_map>
#include <fmt/format.h>
#include <fmt/ostream.h>

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

            enum class FunctionType
            {
                INVALID,
                FUNCTION,
                SCRIPT,
            };

            Compiler(std::string const& source, std::ostream& stderr)
            : source_{&source}
            , stderr_{stderr}
            , scanner_{source}
            , had_error_{false}
            , panic_mode_{false}
            {
                push_state(FunctionType::SCRIPT);

                Local& local = current().locals_[current().local_count_++];
                local.depth = 0;
                local.name.start = "";
                local.name.length = 0;
                local.name.token = {};
            }

            Compiler() = delete;
            Compiler(Compiler const&) = delete;
            Compiler(Compiler&&) = delete;

            Compiler& operator=(Compiler const&) = delete;
            Compiler& operator=(Compiler&&) = delete;

            Function* compile()
            {
                advance();
                while (!match(Token::TOKEN_EOF))
                {
                    declaration();
                }
                auto func = end();
                return had_error_ ? nullptr : func;
            }

            Chunk& chunk() { return current().function_->chunk(); }

        private:
            static constexpr size_t max_locals = static_cast<size_t>(std::numeric_limits<byte>::max()) + 1;

            std::string const* source_;
            std::ostream& stderr_;
            Scanner scanner_;
            
            Token current_;
            Token previous_;
            bool had_error_;
            bool panic_mode_;

            struct State
            {
                Function* function_;
                FunctionType function_type_;
                int local_count_;
                int scope_depth_;
                std::array<Local, max_locals> locals_;

                State()
                : function_{nullptr}
                , function_type_{FunctionType::INVALID}
                , local_count_{0}
                , scope_depth_{0}
                { }

                State(FunctionType type, String* name = nullptr)
                : function_{new Function(name)}
                , function_type_{type}
                , local_count_{0}
                , scope_depth_{0}
                { }

                State(State const&) = default;
                State(State&&) = default;

                State& operator=(State const&) = default;
                State& operator=(State&&) = default;
            };

            std::vector<State> state_;

            void push_state(FunctionType type)
            {
                if (type == FunctionType::SCRIPT)
                    state_.emplace_back(type);
                else
                    state_.emplace_back(type, new String(previous_.token));
            }

            void pop_state()
            {
                assert(!state_.empty());
                state_.pop_back();
            }

            State& current() { return state_.back(); }

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
                else
                    ++line_start;
                auto line_end = source.find("\n", token_start);
                if (line_end == std::string_view::npos)
                    line_end = source.size();

                const auto line = source.substr(line_start, line_end - line_start);
                const auto line_pos = token_start - line_start;
                std::string padding(6, ' ');
                std::string hyphens(line_pos, '-');
                fmt::print(stderr_, "{:4}: {}\n", token.line, line);
                fmt::print(stderr_, "{}{}^\n", padding, hyphens);
            }

            void error_at(Token const& token, std::string const& message)
            {
                if (panic_mode_)
                    return;

                panic_mode_ = true;
                had_error_ = true;
                print_error_location(token);

                fmt::print(stderr_, "Error", token.line);

                if (token.type == Token::TOKEN_EOF) {
                    fmt::print(stderr_, " at end");
                }
                else if (token.type != Token::ERROR) {
                    fmt::print(stderr_, " at '{}'", token.token);
                }

                fmt::print(stderr_, ": {}\n", message);
            }

            byte make_constant(Value value)
            {
                auto constant = chunk().add_constant(value);
                if (constant > std::numeric_limits<byte>::max()) {
                    error("Too many constants in one chunk.");
                    return 0;
                }

                return static_cast<byte>(constant);
            }

            void emit(byte value) { chunk().write(value, previous_.line); }
            void emit(OpCode op) { emit(static_cast<byte>(op)); }
            void emit(OpCode op, byte value) { emit(op); emit(value); }
            void emit_return() { emit(OpCode::OP_RETURN); }
            void emit(Value value) {
                const byte id{make_constant(value)};
                // fmt::print("emit: constant - {}\n", id);
                emit(OpCode::OP_CONSTANT, id); 
            }

            void emit_loop(int loop_start)
            {
                emit(OpCode::OP_LOOP);

                int offset = static_cast<int>(chunk().code().size()) - loop_start + 2;
                if (offset > std::numeric_limits<uint16_t>::max())
                {
                    error("Loop body too large.");
                }
                emit(static_cast<byte>((offset >> 8) & 0xff));
                emit(static_cast<byte>(offset & 0xff));
            }

            void patch_jump(int offset)
            {
                int jump = chunk().code().size() - offset - 2;

                if (jump > std::numeric_limits<uint16_t>::max())
                {
                    error("Too much code to jump over.");
                }

                chunk().code()[offset] = static_cast<byte>((jump >> 8) & 0xff);
                chunk().code()[offset + 1] = static_cast<byte>(jump & 0xff);
            }

            int emit_jump(OpCode op)
            {
                emit(op);
                emit(0xff);
                emit(0xff);
                return chunk().code().size() - 2;
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

            Function* end()
            { 
                emit_return(); 

#ifdef DEBUG_PRINT_CODE
                if (!had_error_)
                    disassemble(
                        stderr_, 
                        chunk(), 
                        current().function_->name() 
                            ? current().function_->name()->view()
                            : "<script>"
                    );
#endif
                auto func = current().function_;
                pop_state();
                return func;
            }

            void begin_scope()
            {
                ++current().scope_depth_;
            }

            void end_scope()
            {
                --current().scope_depth_;

                while (
                    current().local_count_ > 0
                    && current().locals_[current().local_count_ - 1].depth > current().scope_depth_
                    )
                {
                    emit(OpCode::OP_POP);
                    --current().local_count_;
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

            void function(FunctionType type)
            {
                push_state(type);
                begin_scope();
                consume(Token::LEFT_PAREN, "Expect '(' after function name.");
                if (!check(Token::RIGHT_PAREN))
                {
                    int arity = 0;
                    do
                    {
                        ++arity;
                        if (arity > Function::max_parameters) {
                            error_at_current("Cannot have more than 255 parameters.");
                        }

                        auto constant = parse_variable("Expect parameter name");
                        define_variable(constant);
                    } while (match(Token::COMMA));

                    current().function_->arity(arity);
                }
                consume(Token::RIGHT_PAREN, "Expect ')' after parameters.");
                consume(Token::LEFT_BRACE, "Expect '{' after function body.");
                block();

                auto function = end();

                emit(OpCode::OP_CONSTANT, make_constant(Value{function}));
            }

            void fun_declaration()
            {
                byte global = parse_variable("Expect function name.");
                mark_initialized();
                function(FunctionType::FUNCTION);
                define_variable(global);
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

            void for_statement()
            {
                begin_scope();
                consume(Token::LEFT_PAREN, "Expect '(' after 'for'.");

                // initializer clause
                if (match(Token::SEMICOLON)) {
                    // no initializer
                } else if (match(Token::VAR)) {
                    var_declaration();
                } else {
                    expression_statement();
                }

                int loop_start = static_cast<int>(chunk().code().size());

                // condition clause
                int exit_jump = -1;
                if (!match(Token::SEMICOLON))
                {
                    expression();
                    consume(Token::SEMICOLON, "Expect ';' after loop condition.");

                    exit_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE);
                    emit(OpCode::OP_POP);
                }

                // increment clause
                if (!match(Token::RIGHT_PAREN))
                {
                    int body_jump = emit_jump(OpCode::OP_JUMP);
                    int increment_start = static_cast<int>(chunk().code().size());

                    expression();
                    emit(OpCode::OP_POP);

                    consume(Token::RIGHT_PAREN, "Expect ')' after for clauses.");

                    emit_loop(loop_start);
                    loop_start = increment_start;
                    patch_jump(body_jump);
                }

                statement();
                emit_loop(loop_start);

                if (exit_jump != -1)
                {
                    patch_jump(exit_jump);
                    emit(OpCode::OP_POP);
                }

                end_scope();
            }

            void if_statement()
            {
                consume(Token::LEFT_PAREN, "Expect '(' after 'if'.");
                expression();
                consume(Token::RIGHT_PAREN, "Expect ')' after condition.");

                int then_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE);
                emit(OpCode::OP_POP);

                statement();

                int else_jump = emit_jump(OpCode::OP_JUMP);
                patch_jump(then_jump);
                emit(OpCode::OP_POP);

                if (match(Token::ELSE))
                    statement();
                patch_jump(else_jump);
            }

            void print_statement()
            {
                expression();
                consume(Token::SEMICOLON, "Expect ';' after value.");
                emit(OpCode::OP_PRINT);
            }

            void while_statement()
            {
                int loop_start = static_cast<int>(chunk().code().size());
                consume(Token::LEFT_PAREN, "Expect '(' after 'while'.");
                expression();
                consume(Token::RIGHT_PAREN, "Expect ')' after condition.");

                int exit_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE);
                emit(OpCode::OP_POP);
                statement();
                emit_loop(loop_start);
                patch_jump(exit_jump);
                emit(OpCode::OP_POP);
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
                if (match(Token::FUN)) {
                    fun_declaration();
                }
                else if (match(Token::VAR)) {
                    var_declaration();
                }
                else {
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
                else if (match(Token::FOR))
                {
                    for_statement();
                }
                else if (match(Token::IF))
                {
                    if_statement();
                }
                else if (match(Token::WHILE))
                {
                    while_statement();
                }
                else if (match(Token::LEFT_BRACE))
                {
                    begin_scope();
                    block();
                    end_scope();
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
                    arg = identifier_constant(name);
                    get_op = OpCode::OP_GET_GLOBAL;
                    set_op = OpCode::OP_SET_GLOBAL;
                }

                if (can_assign && match(Token::EQUAL))
                {
                    expression();
                    emit(set_op, static_cast<byte>(arg));
                }
                else
                {
                    emit(get_op, static_cast<byte>(arg));
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
                for (int i = current().local_count_ - 1; i >= 0; --i)
                {
                    if (identifiers_equal(name, current().locals_[i].name))
                    {
                        if (current().locals_[i].depth == -1)
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
                if (current().local_count_ == max_locals)
                {
                    error("Too many local variables in function.");
                    return;
                }

                Local& local = current().locals_[current().local_count_++];
                local.name = name;
                local.depth = -1;
            }

            void declare_variable()
            {
                if (current().scope_depth_ == 0)
                    return;
                
                Token name{previous_};
                for (int i = current().local_count_ - 1; i >= 0; --i)
                {
                    auto& local = current().locals_[i];
                    if (local.depth != -1 && local.depth < current().scope_depth_)
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
                if (current().scope_depth_ > 0)
                    return 0;

                return identifier_constant(previous_);
            }

            void mark_initialized()
            {
                if (current().scope_depth_ == 0)
                    return;

                current().locals_[current().local_count_ - 1].depth = current().scope_depth_;
            }

            void define_variable(byte global)
            {
                if (current().scope_depth_ > 0)
                {
                    mark_initialized();
                    return;
                }

                emit(OpCode::OP_DEFINE_GLOBAL, global);
            }

            void and_(bool can_assign)
            {
                ignore(can_assign);

                int end_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE);
                emit(OpCode::OP_POP);
                parse_precedence(Precedence::AND);
                patch_jump(end_jump);
            }

            void or_(bool can_assign)
            {
                ignore(can_assign);

                int else_jump = emit_jump(OpCode::OP_JUMP_IF_FALSE);
                int end_jump = emit_jump(OpCode::OP_JUMP);

                patch_jump(else_jump);
                emit(OpCode::OP_POP);

                parse_precedence(Precedence::OR);
                patch_jump(end_jump);
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

                    RULE(AND, nullptr, &Compiler::and_, AND),
                    RULE(OR, nullptr, &Compiler::or_, OR),

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
    Function* compile(std::string const& source, std::ostream& stderr)
    {
        Compiler compiler{source, stderr};
        return compiler.compile();
    }
}