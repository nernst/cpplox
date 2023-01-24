#pragma once
#include "common.hpp"
#include <cassert>
#include <type_traits>
#include <variant>
#include <vector>
#include <iosfwd>

namespace lox {
    class Object;

    template<typename T>
    inline constexpr bool is_obj_ptr_convertible_v = 
        std::is_pointer_v<T>
        && std::is_convertible_v<T, Object*>;

    namespace detail {
        std::string_view obj_type_name(Object const* obj);
        ObjectType obj_type(Object const* obj);
    }
}

#ifdef LOX_NAN_BOXING
#error "not implemented"
#else
#include "./detail/value_base_variant.hpp"
#endif

namespace lox
{
    #ifdef LOX_NAN_BOXING
    #else
    using Value = ::lox::detail::ValuBaseVariant;
    #endif

    void print_value(Value const& value);
    void print_value(std::ostream& stream, Value const& value);
}
 