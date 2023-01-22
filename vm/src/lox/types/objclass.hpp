#pragma once

#include "../object.hpp"

namespace lox {

    class ObjClass : public Object
    {
    public:
        explicit ObjClass(String* name)
        : Object{}
        , name_{name}
        {
            assert(name);
        }

        ObjClass() = delete;
        ObjClass(ObjClass const&) = delete;
        ObjClass(ObjClass&&) = delete;

        ObjClass& operator=(ObjClass const&) = delete;
        ObjClass& operator=(ObjClass&&) = delete;

        ObjectType type() const override { return ObjectType::OBJCLASS; }  
        const char* type_name() const override { return "ObjClass"; }

        String* name() const { return name_; }

    private:
        String* name_;

        void gc_blacken(GC& gc) override;
    };

}
