#include "lox/object.hpp"
#include "lox/gc.hpp"
#include "lox/map.hpp"
#include "fmt/ostream.h"
#include <iostream>

namespace lox
{
    namespace {

        lox::Map& strings()
        {
            static lox::Map strings_{Tracking::WEAK};
            return strings_;
        }
    }

    template<class StringType>
    String* String::do_create(StringType&& value)
    {
        auto& strs = strings();
        Value v;
        if (strs.get(std::string_view{value}, v))
            return v.get<String*>();
        
        String* s = new String(std::forward<StringType>(value));
        strs.add(s, Value{s});
        return s;

    }


    String* String::create(const char* value)
    {
        std::string_view view {value};
        return do_create(view);
    }

    String* String::create(std::string_view value)
    {
        return do_create(value);
    }

    String* String::create(std::string&& value)
    {
        return do_create(std::forward<decltype(value)>(value));
    }

    Object::Object()
    : gc_next_{GC::instance().root_.exchange(this)}
    , gc_marked_{false}
    { }

    void Object::gc_blacken(GC& gc)
    {
        ignore(gc);

        #ifdef DEBUG_LOG_GC
        fmt::print(std::cerr, "{} blacken ", static_cast<void*>(this));
        print_object(std::cerr, *this);
        fmt::print(std::cerr, "\n");
        #endif
    }

    Object::~Object() { }

    void print_object(std::ostream& os, Object const& object)
    {
        switch(object.type())
        {
            case ObjectType::STRING:
                fmt::print(os, "{}", static_cast<String const&>(object).c_str());
                break;

            case ObjectType::NATIVE_FUNCTION:
                fmt::print(
                    os, 
                    "<native fn @{}>", 
                    static_cast<void const*>(&object)
                );
                break;

            case ObjectType::FUNCTION:
            {
                Function const& fun = static_cast<Function const&>(object);
                if (fun.name())
                    fmt::print(os, "<fn {}>", fun.name()->view());
                else
                    fmt::print(os, "<script>");
                break;
            }

            case ObjectType::CLOSURE:
            {
                Closure const& closure = static_cast<Closure const&>(object);
                print_object(os, *closure.function());
                break;
            }

            case ObjectType::OBJUPVALUE:
                fmt::print(os, "<upvalue>");
                break;

            default:
                unreachable();
                break;
        }

    }

    void ObjUpvalue::gc_blacken(GC& gc)
    {
        Object::gc_blacken(gc);

        gc.mark_value(closed_);
    }

    void Function::gc_blacken(GC& gc)
    {
        Object::gc_blacken(gc);

        gc.mark_object(name_);

        auto const& array = chunk_.constants();
        for (size_t i = 0; i != array.size(); ++i)
        {
            Value v{array[i]};
            gc.mark_value(v);
        }
    }

    void Closure::gc_blacken(GC& gc)
    {
        Object::gc_blacken(gc);

        gc.mark_object(function_);

        auto upvalues = upvalues_.get();
        if (!upvalues)
            return;

        for (size_t i = 0; i != upvalue_count_; ++i)
            gc.mark_object(upvalues[i]);

    }
}
