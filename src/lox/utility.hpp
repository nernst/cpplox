#pragma once

#include <iostream>

namespace lox {

template<class... Types>
void ignore_unused(Types&&...) { }

template<class>
constexpr bool always_false_v = false;

}

