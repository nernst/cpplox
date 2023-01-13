#include "lox/object.hpp"
#include <atomic>

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

    void print_object(Object const& object)
    {
        switch(object.type())
        {
            case ObjectType::STRING:
                fmt::print("{}", static_cast<String const&>(object).c_str());
                break;
            default:
                unreachable();
                break;
        }

    }
}
