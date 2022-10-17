#include "lox/object.hpp"

namespace lox
{
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
