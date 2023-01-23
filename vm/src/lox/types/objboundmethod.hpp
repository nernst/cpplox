#pragma once

#include "../object.hpp"

namespace lox {

    class ObjBoundMethod : public Object
    {
    public:
        template<class ValueType>
        ObjBoundMethod(ValueType&& receiver, Closure* method)
        : Object{}
        , receiver_{std::forward<ValueType>(receiver)}
        , method_{method}
        {
            assert(method);
        }

        ObjBoundMethod() = delete;
        ObjBoundMethod(ObjBoundMethod const&) = delete;
        ObjBoundMethod(ObjBoundMethod&&) = delete;

        ObjBoundMethod& operator=(ObjBoundMethod const&) = delete;
        ObjBoundMethod& operator=(ObjBoundMethod&&) = delete;

        Value& receiver() { return receiver_; }
        Value const& receiver() const { return receiver_; }

        Closure* method() { return method_; }
        Closure const* method() const { return method_; }

        ObjectType type() const override { return ObjectType::OBJBOUNDMETHOD; }
        const char* type_name() const override { return "ObjBoundMethod"; }

    private:
        Value receiver_;
        Closure* method_;

        void gc_blacken(GC& gc) override;
    };

}
