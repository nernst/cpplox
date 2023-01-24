#include "lox/compiler.hpp"
#ifdef DEBUG_PRINT_CODE
#include "lox/debug.hpp"
#endif
#include "lox/scanner.hpp"
#include "lox/object.hpp"
#include "lox/gc.hpp"
#include <array>
#include <iterator>
#include <list>
#include <sstream>
#include <unordered_map>
#include <fmt/format.h>
#include <fmt/ostream.h>

#ifdef DEBUG_LOG_GC
#include <iostream>
#endif

// #define TRACE_COMPILE


namespace lox
{
    namespace
    {
        class Compiler
        {
            struct Local
            {
                Token name;
                int depth = 0;
                bool is_captured = false;
            };

            struct Upvalue
            {
                Upvalue(byte index, bool is_local)
                : index{index}
                , is_local{is_local}
                { }

                Upvalue() = default;
                Upvalue(Upvalue const&) = default;
                Upvalue(Upvalue&&) = default;

                Upvalue& operator=(Upvalue const&) = default;
                Upvalue& operator=(Upvalue&&) = default;

                byte index = 0;
                bool is_local = false;
            };

            struct Class
            {
                Class* enclosing = nullptr;
                bool has_super_class = false;
            };

        public:

            enum class FunctionType
            {
                INVALID,
                FUNCTION,
                SCRIPT,
                METHOD,
                INITIALIZER,
            };

            Compiler(std::string const& source, std::ostream& stderr)
            : source_{&source}
            , stderr_{stderr}
            , scanner_{source}
            , had_error_{false}
            , panic_mode_{false}
            {
                push_state(FunctionType::SCRIPT);
            }

            Compiler() = delete;
            Compiler(Compiler const&) = delete;
            Compiler(Compiler&&) = delete;

            Compiler& operator=(Compiler const&) = delete;
            Compiler& operator=(Compiler&&) = delete;

            gcroot<Function> compile()
            {
                advance();
                while (!match(Token::TOKEN_EOF))
                {
                    declaration();
                }
                auto func = end();
                return had_error_ ? gcroot<Function>{} : func;
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
            Class* current_class_ = nullptr;

            struct State
            {
                State* enclosing_ = nullptr;
                gcroot<Function> function_;
                FunctionType function_type_ = FunctionType::INVALID;
                int scope_depth_ = 0;
                std::vector<Upvalue> upvalues_;
                std::vector<Local> locals_;

                State() = default;

                State(FunctionType type, String* name = nullptr, State* enclosing = nullptr)
                : enclosing_{enclosing}
                , function_{new Function(name)}
                , function_type_{type}
                { }

                State(State const&) = default;
                State(State&&) = default;

                State& operator=(State const&) = default;
                State& operator=(State&&) = default;

                friend std::ostream& operator<<(std::ostream& os, State const& state)
                {
                    os << "{Function ";
                    if (state.function_type_ == FunctionType::INVALID || !state.function_)
                    {
                        os << "!!INVALID!!" << std::endl;
                    }
                    else
                    {
                        os << (state.function_->name() ? state.function_->name()->view() : "<script>")
                            << ", scope depth: " << state.scope_depth_
                            << ", enclosing: " << static_cast<void*>(state.enclosing_)
                            << ",\n\tlocals: " << state.locals_.size() << " [";
                        
                        for (auto&& local : state.locals_)
                        {
                            os << "{" << local.name.token 
                                << ", depth=" << local.depth << "}, ";
                        }
                        os << "],\n\tupvalues: [";

                        for (auto&& upvalue : state.upvalues_)
                        {
                            os << "{index=" << static_cast<unsigned>(upvalue.index)
                                << ", is_local=" << upvalue.is_local << "}, ";
                        }
                        os << "]";
                }
                    os << "}";

                    return os;
                }
            };

            std::list<State> state_;

            State& push_state(FunctionType type)
            {
                if (type == FunctionType::SCRIPT)
                    state_.emplace_back(type);
                else
                {
                    assert(!state_.empty());
                    state_.emplace_back(type, String::create(previous_.token), &state_.back());
                }

                Token token{type == FunctionType::FUNCTION ? std::string_view{} : "this"};
                current().locals_.emplace_back(std::move(token), 0, false);

                #ifdef TRACE_COMPILE
                stderr_ << "** push state: depth=" << state_.size()
                    << ", type = " << (type == FunctionType::SCRIPT ? "SCRIPT" : "FUNCTION") << std::endl;
                #endif

                return state_.back();
            }

            std::vector<Upvalue> pop_state()
            {
                assert(!state_.empty());
                #ifdef TRACE_COMPILE
                stderr_ << "** pop state: depth=" << state_.size() << std::endl;
                #endif
                auto ret = std::move(state_.back().upvalues_);
                state_.pop_back();
                return ret;
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

            void emit_return()
            { 
                if (current().function_type_ == FunctionType::INITIALIZER)
                {
                    emit(OpCode::OP_GET_LOCAL, 0);
                }
                else
                {
                    emit(OpCode::OP_NIL);
                }
                emit(OpCode::OP_RETURN);
            }

            void emit(Value value)
            {
                const byte id{make_constant(value)};
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

            gcroot<Function> end()
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
                auto func = std::move(current().function_);

#ifdef DEBUG_PRINT_CODE
                stderr_ << state_.back() << std::endl;
#endif
#ifdef TRACE_COMPILE
                stderr_ << "* end() - upvalues: " << current().upvalues_.size() << std::endl;
#endif
                func->upvalues(current().upvalues_.size());
                return func;
            }

            void begin_scope()
            {
                ++current().scope_depth_;
            }

            void end_scope()
            {
                auto& scope = current();
                --scope.scope_depth_;

                size_t i = scope.locals_.size();
                for ( ; i != 0; --i)
                {
                    auto& local = scope.locals_[i - 1];
                    if (local.depth <= scope.scope_depth_)
                        break;
                    
                    if (local.is_captured)
                        emit(OpCode::OP_CLOSE_UPVALUE);
                    else
                        emit(OpCode::OP_POP);
                }

                auto iter = std::begin(scope.locals_);
                std::advance(iter, i);
                scope.locals_.erase(iter, std::end(scope.locals_));
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
                State& state = push_state(type);

                begin_scope();
                consume(Token::LEFT_PAREN, "Expect '(' after function name.");
                if (!check(Token::RIGHT_PAREN))
                {
                    unsigned arity = 0;
                    do
                    {
                        ++arity;
                        if (arity > Function::max_parameters) {
                            error_at_current("Cannot have more than 255 parameters.");
                        }

                        auto constant = parse_variable("Expect parameter name");
                        define_variable(constant);
                    } while (match(Token::COMMA));

                    state.function_->arity(arity);
                }
                consume(Token::RIGHT_PAREN, "Expect ')' after parameters.");
                consume(Token::LEFT_BRACE, "Expect '{' after function body.");
                block();

                auto function = end();
                auto upvalues = pop_state();

                emit(OpCode::OP_CLOSURE, make_constant(Value{function.get()}));
                for (auto&& upvalue : upvalues)
                {
                    emit(static_cast<byte>(upvalue.is_local ? 1 : 0));
                    emit(upvalue.index);
                }
            }

            void method()
            {
                consume(Token::IDENTIFIER, "Expect method name.");
                auto constant = identifier_constant(previous_);
                FunctionType type{FunctionType::METHOD};
                if (previous_.token == "init")
                    type = FunctionType::INITIALIZER;
                function(type);
                emit(OpCode::OP_METHOD, constant);
            }

            void class_declaration()
            {
                consume(Token::IDENTIFIER, "Expect class name.");
                auto class_name = previous_;
                auto name_constant = identifier_constant(previous_);
                declare_variable();

                emit(OpCode::OP_CLASS, name_constant);
                define_variable(name_constant);

                Class klass{current_class_};
                current_class_ = &klass;

                if (match(Token::LESS))
                {
                    consume(Token::IDENTIFIER, "Expect superclass name.");
                    variable(false);
                    if (identifiers_equal(class_name, previous_))
                    {
                        error("A cklass cannot inherit from itself.");
                    }

                    begin_scope();
                    add_local(Token{"super"});
                    define_variable(0);

                    named_variable(class_name, false);
                    emit(OpCode::OP_INHERIT);
                    klass.has_super_class = true;
                }

                named_variable(class_name, false); // push class name

                consume(Token::LEFT_BRACE, "Expect '{' before class body.");
                while (!check(Token::RIGHT_BRACE) && !check(Token::TOKEN_EOF))
                {
                    method();
                }
                consume(Token::RIGHT_BRACE, "Expect '}' after class body.");
                emit(OpCode::OP_POP); // pop class name
                if (klass.has_super_class)
                {
                    end_scope();
                }

                current_class_ = klass.enclosing;
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

            void return_statement()
            {
                if (current().function_type_ == FunctionType::SCRIPT)
                {
                    error("Cannot return from top-level code.");
                }

                if (match(Token::SEMICOLON))
                {
                    emit_return();
                }
                else
                {
                    if (current().function_type_ == FunctionType::INITIALIZER)
                    {
                        error("Cannot return a value from an initializer.");
                    }
                    expression();
                    consume(Token::SEMICOLON, "Expect ';' after return value.");
                    emit(OpCode::OP_RETURN);
                }
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
                if (match(Token::CLASS)) {
                    class_declaration();
                }
                else if (match(Token::FUN)) {
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
                else if (match(Token::RETURN))
                {
                    return_statement();
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
                    String::create(std::string_view{previous_.start + 1, previous_.length - 2})
                });
            }

            void named_variable(Token const& name, bool can_assign)
            {
                #ifdef TRACE_COMPILE
                stderr_ << "** named_variable: " << name.token << ", can_assign: " << can_assign << std::endl;
                #endif

                OpCode get_op, set_op;
                int arg = resolve_local(name);
                if (arg != -1)
                {
                    #ifdef TRACE_COMPILE
                    stderr_ << "*** resolve_local: " << arg << std::endl;
                    #endif
                    get_op = OpCode::OP_GET_LOCAL;
                    set_op = OpCode::OP_SET_LOCAL;
                }
                else if ((arg = resolve_upvalue(name)) != -1)
                {
                    #ifdef TRACE_COMPILE
                    stderr_ << "*** resolve_upvalue: " << arg << std::endl;
                    #endif
                    get_op = OpCode::OP_GET_UPVALUE;
                    set_op = OpCode::OP_SET_UPVALUE;
                }
                else
                {
                    arg = identifier_constant(name);
                    #ifdef TRACE_COMPILE
                    stderr_ << "*** identifier_constant: " << arg << std::endl;
                    #endif
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

            void super_(bool can_assign)
            {
                ignore(can_assign);

                if (current_class_ == nullptr)
                {
                    error("Cannot use 'super' outside of a class.");
                }
                else if (!current_class_->has_super_class)
                {
                    error("Cannot user 'super' in a class with no superclass.");
                }

                consume(Token::DOT, "Expect '.' after 'super'.");
                consume(Token::IDENTIFIER, "Expect superclass method name.");
                auto name = identifier_constant(previous_);
                named_variable(Token{"this"}, false);
                named_variable(Token{"super"}, false);
                emit(OpCode::OP_GET_SUPER, name);
            }

            void this_(bool can_assign)
            {
                ignore(can_assign);

                if (current_class_ == nullptr)
                {
                    error("Cannot use 'this' outside of a class.");
                    return;
                }

                variable(false);
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

            void call(bool can_assign)
            {
                ignore(can_assign);

                byte arg_count = argument_list();
                emit(OpCode::OP_CALL, arg_count);
            }

            void dot(bool can_assign)
            {
                consume(Token::IDENTIFIER, "Expect property name after '.'.");
                auto name = identifier_constant(previous_);

                if (can_assign && match(Token::EQUAL))
                {
                    expression();
                    emit(OpCode::OP_SET_PROPERTY, name);
                }
                else if (match(Token::LEFT_PAREN))
                {
                    auto arg_count = argument_list();
                    emit(OpCode::OP_INVOKE, name);
                    emit(arg_count);
                }
                else
                {
                    emit(OpCode::OP_GET_PROPERTY, name);
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
                return make_constant(Value{String::create(name.token)});
            }

            static bool identifiers_equal(Token const& lhs, Token const& rhs)
            {
                return lhs.token == rhs.token;
            }

            int resolve_local(Token const& name, State* scope = nullptr)
            {
                if (scope == nullptr)
                    scope = &current();

                #ifdef TRACE_COMPILE
                stderr_ << "** resolve_local: " << name.token << std::endl << *scope << std::endl;
                #endif

                for (int i = static_cast<int>(scope->locals_.size()) - 1; i >= 0; --i)
                {
                    if (identifiers_equal(name, scope->locals_[i].name))
                    {
                        if (scope->locals_[i].depth == -1)
                        {
                            error("Cannot read local variable in its own initializer.");
                        }
                        #ifdef TRACE_COMPILE
                        stderr_ << "*** found: " << i << std::endl;
                        #endif
                        return i;
                    }
                }

                #ifdef TRACE_COMPILE
                stderr_ << "*** NOT FOUND" << std::endl;
                #endif

                return -1;
            }

            int add_upvalue(State& state, byte index, bool is_local)
            {
                #ifdef TRACE_COMPILE
                stderr_ << "** add_upvalue: index=" << static_cast<unsigned>(index) 
                    << ", is_local=" << is_local << std::endl;
                #endif

                auto& upvalues = state.upvalues_;
                for (size_t i = 0; i < upvalues.size(); ++i)
                {
                    if (upvalues[i].index == index && upvalues[i].is_local == is_local)
                    {
                        #ifdef TRACE_COMPILE
                        stderr_ << "*** found at: " << i << std::endl;
                        #endif
                        return static_cast<int>(i);
                    }
                }

                if (upvalues.size() == Function::max_upvalues)
                {
                    error("Too many clsoure variables in function.");
                    return 0;
                }

                upvalues.emplace_back(index, is_local);
                #ifdef TRACE_COMPILE
                stderr_ << "*** added upvalue at: " << (upvalues.size() - 1) << std::endl;
                #endif
                return static_cast<int>(upvalues.size() - 1);
            }

            int resolve_upvalue(Token const& name, State* scope = nullptr)
            {
                if (scope == nullptr)
                    scope = &current();

                auto parent = scope->enclosing_;
                if (parent == nullptr)
                    return -1;
                
                int local = resolve_local(name, parent);
                if (local != -1)
                {
                    parent->locals_[local].is_captured = true;
                    return add_upvalue(*scope, static_cast<byte>(local), true);
                }

                int upvalue = resolve_upvalue(name, parent);
                if (upvalue != -1)
                {
                    return add_upvalue(*scope, static_cast<byte>(upvalue), false);
                }

                return -1;
            }

            void add_local(Token const& name)
            {
                if (current().locals_.size() >= max_locals)
                {
                    error("Too many local variables in function.");
                    return;
                }

                current().locals_.emplace_back(name, -1, false);
            }

            void declare_variable()
            {
                if (current().scope_depth_ == 0)
                    return;
                
                Token name{previous_};
                for (auto&& local : current().locals_)
                {
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

                if (!current().locals_.empty())
                    current().locals_.back().depth = current().scope_depth_;
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

            byte argument_list()
            {
                byte arg_count{0};
                if (!check(Token::RIGHT_PAREN))
                {
                    do
                    {
                        expression();
                        if (arg_count == Function::max_parameters)
                            error(fmt::format("Cannot have more than {} arguments.", Function::max_parameters));
                        ++arg_count;
                    } while (match(Token::COMMA));
                }

                consume(Token::RIGHT_PAREN, "Expect ')' after arguments.");
                return arg_count;
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
                    RULE(LEFT_PAREN, &Compiler::grouping, &Compiler::call, CALL),
                    RULE(DOT, nullptr, &Compiler::dot, CALL),
                    RULE(MINUS, &Compiler::unary, &Compiler::binary, TERM),
                    RULE(PLUS, nullptr, &Compiler::binary, TERM),
                    RULE(SLASH, nullptr, &Compiler::binary, FACTOR),
                    RULE(STAR, nullptr, &Compiler::binary, FACTOR),
                    RULE(NUMBER, &Compiler::number, nullptr, NONE),
                    RULE(IDENTIFIER, &Compiler::variable, nullptr, NONE),
                    RULE(STRING, &Compiler::string, nullptr, NONE),
                    RULE(SUPER, &Compiler::super_, nullptr, NONE),
                    RULE(THIS, &Compiler::this_, nullptr, NONE),
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

    gcroot<Function> compile(std::string const& source, std::ostream& stderr)
    {
        Compiler compiler{source, stderr};
        return compiler.compile();
    }
}
