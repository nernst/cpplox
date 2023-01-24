#pragma once

#include "../object.hpp"
#include "../map.hpp"
#include "objclass.hpp"

namespace lox {

    class ObjInstance : public Object
    {
    public:
        explicit ObjInstance(ObjClass* klass)
        : Object{}
        , class_{klass}
        , fields_{Tracking::HELD}
        {
            assert(klass);
        }

        ObjInstance() = delete;
        ObjInstance(ObjInstance const&) = delete;
        ObjInstance(ObjInstance&&) = delete;

        ObjInstance& operator=(ObjInstance const&) = delete;
        ObjInstance& operator=(ObjInstance&&) = delete;

        ObjClass* obj_class() const { return class_; }

        Map& fields() { return fields_; }
        Map const& fields() const { return fields_; }

        ObjectType type() const override { return ObjectType::OBJINSTANCE; }
        std::string_view type_name() const override { return "ObjInstance"; }

    private:
        ObjClass* class_;
        Map fields_;

        void gc_blacken(GC& gc) override;
    };
}
