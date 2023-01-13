#include "lox/map.hpp"
#include <iostream>

namespace lox {

    std::ostream& operator<<(std::ostream& os, Map const& map)
    {
        os << "{";

        for (size_t i = 0; i != map.capacity_; ++i)
        {
            Map::Entry const* p = map.entries_.get() + i;
            if (!p->key)
                continue;

            os << p->key->view() << ": ";
            print_value(os, p->value);
            os << ", ";
        }

        os << "}";

        return os;
    }
}
