#include "lox/object.hpp"
#include <atomic>
#include "fmt/ostream.h"

namespace lox
{
    namespace {
        static std::atomic<Object*> gc_root_ = nullptr;
    }

    Object* Object::gc_root()
    { return gc_root_.load(); }

    void Object::run_gc()
    {
        Object* root{nullptr};
        root = gc_root_.exchange(root);

        while (root)
        {
            Object* next = root->gc_next_;
            delete root;
            root = next;
        }
    }

    Object::Object()
    : gc_next_{gc_root_.exchange(this)}
    { }

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
}
