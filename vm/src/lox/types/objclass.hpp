#pragma once

#include "../object.hpp"
#include "../map.hpp"

namespace lox {

    class ObjClass : public Object
    {
    public:
        explicit ObjClass(String* name)
        : Object{}
        , name_{name}
        , methods_{Tracking::HELD}
        {
            assert(name);
        }

        ObjClass() = delete;
        ObjClass(ObjClass const&) = delete;
        ObjClass(ObjClass&&) = delete;

        ObjClass& operator=(ObjClass const&) = delete;
        ObjClass& operator=(ObjClass&&) = delete;

        ObjectType type() const override { return ObjectType::OBJCLASS; }  
        std::string_view type_name() const override { return "ObjClass"; }

        Map& methods() { return methods_; }
        Map const& methods() const { return methods_; }

        String* name() const { return name_; }

    private:
        String* name_;
        Map methods_;

        void gc_blacken(GC& gc) override;
    };

}
